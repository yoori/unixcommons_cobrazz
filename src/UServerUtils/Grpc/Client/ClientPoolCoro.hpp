#ifndef GRPC_CLIENT_CLIENT_POOL_CORO_H_
#define GRPC_CLIENT_CLIENT_POOL_CORO_H_

// STD
#include <memory>
#include <unordered_map>

// GRPC
#include <grpc/impl/codegen/connectivity_state.h>
#include <grpcpp/impl/codegen/proto_utils.h>
#include <grpcpp/grpcpp.h>

// USERVER
#include <userver/engine/async.hpp>
#include <userver/engine/shared_mutex.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task_processor.hpp>

// THIS
#include <Generics/Uncopyable.hpp>
#include <Logger/Logger.hpp>
#include <UServerUtils/Grpc/Client/ClientCoro.hpp>
#include <UServerUtils/Grpc/Client/ClientObserver.hpp>
#include <UServerUtils/Grpc/Client/Config.hpp>
#include <UServerUtils/Grpc/Client/ConfigPoolCoro.hpp>
#include <UServerUtils/Grpc/Client/Factory.hpp>
#include <UServerUtils/Grpc/Client/FactoryObserver.hpp>
#include <UServerUtils/Grpc/Client/Types.hpp>
#include <UServerUtils/Utils.hpp>

namespace UServerUtils::Grpc::Client
{

namespace Aspect
{

constexpr const char CLIENT_POOL_CORO[] = "CLIENT_POOL_CORO";

} // namespace Aspect

namespace Internal
{

template<class RpcServiceMethodConcept, class = void>
class ClientPoolCoroImpl final
{
};

template<class RpcServiceMethodConcept>
class ClientPoolCoroImpl<
  RpcServiceMethodConcept,
  Internal::has_bidi_streaming_t<RpcServiceMethodConcept>> final
  : private Generics::Uncopyable
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using Traits = Internal::Traits<RpcServiceMethodConcept>;
  using Request = typename Traits::Request;
  using RequestPtr = std::unique_ptr<Request>;
  using WriteResult = typename ClientCoro<RpcServiceMethodConcept>::WriteResult;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

private:
  using ClientPtr = ClientCoroPtr<RpcServiceMethodConcept>;
  using Position = std::size_t;
  using ClientInfo = std::pair<ClientPtr, Position>;
  using Clients = std::unordered_map<ClientId, ClientInfo>;
  using ClientIds = std::vector<ClientId>;
  using Counter = std::atomic<std::uint32_t>;
  using Mutex = userver::engine::SharedMutex;

public:
  ClientPoolCoroImpl(Logger* logger)
    : logger_(ReferenceCounting::add_ref(logger))
  {
    client_ids_.reserve(10000);
  }

  ~ClientPoolCoroImpl() = default;

  WriteResult write(
    RequestPtr&& request,
    const std::size_t timeout)  noexcept
  {
    try
    {
      if (!request)
      {
        Stream::Error stream;
        stream << FNS
               << "request is null";
        throw Exception(stream);
      }

      const auto number = counter_.fetch_add(
        1,
        std::memory_order_relaxed);

      ClientPtr client;
      {
        std::shared_lock lock(mutex_client_);
        const auto size = client_ids_.size();
        if (size == 0)
        {
          return WriteResult(Status::InternalError, {});
        }

        const auto client_id = client_ids_[number % size];
        const auto it = clients_.find(client_id);
        if (it == std::end(clients_))
          return WriteResult(Status::InternalError, {});

        client = it->second.first;
      }

      return client->write(std::move(request), timeout);
    }
    catch (const eh::Exception &exc)
    {
      try
      {
        Stream::Error stream;
        stream << FNS
               << exc.what();
        logger_->error(stream.str(), Aspect::CLIENT_POOL_CORO);
      }
      catch (...)
      {
      }
    }
    catch (...)
    {
      try
      {
        Stream::Error stream;
        stream << FNS
               << "Unknown error";
        logger_->error(stream.str(), Aspect::CLIENT_POOL_CORO);
      }
      catch (...)
      {
      }
    }

    return WriteResult(Status::InternalError, {});
  }

  void emplace(ClientPtr&& client) noexcept
  {
    try
    {
      const auto client_id = client->client_id();

      std::unique_lock lock(mutex_client_);
      const auto size = client_ids_.size();
      const auto result = clients_.try_emplace(
        client_id,
        std::move(client),
        size);
      if (!result.second)
      {
        Stream::Error stream;
        stream << FNS
               << "Logic error. Already existing client_id="
               << client_id;
        throw Exception(stream.str());
      }
      client_ids_.emplace_back(client_id);
    }
    catch (const eh::Exception& exc)
    {
      try
      {
        Stream::Error stream;
        stream << FNS
               << exc.what();
        logger_->error(stream.str(), Aspect::CLIENT_POOL_CORO);
      }
      catch (...)
      {
      }
    }
    catch (...)
    {
      try
      {
        Stream::Error stream;
        stream << FNS
               << ": Unknown error";
        logger_->error(stream.str(), Aspect::CLIENT_POOL_CORO);
      }
      catch (...)
      {
      }
    }
  }

  ClientPtr remove(const ClientId client_id) noexcept
  {
    try
    {
      std::unique_lock lock(mutex_client_);
      auto it_remove = clients_.find(client_id);
      if (it_remove == std::end(clients_))
      {
        return {};
      }
      auto client = it_remove->second.first;

      const auto size = client_ids_.size();
      const auto position = it_remove->second.second;
      if (position >= size)
      {
        Stream::Error stream;
        stream << FNS
               << ": Logic error. position="
               << position
               << " is larger then "
               << size;
        logger_->error(stream.str(), Aspect::CLIENT_POOL_CORO);
        return client;
      }

      clients_.erase(it_remove);
      std::swap(client_ids_[position], client_ids_[size - 1]);
      client_ids_.pop_back();

      if (!client_ids_.empty() && position != size - 1)
      {
        auto it_change = clients_.find(client_ids_[position]);
        if (it_change == std::end(clients_))
        {
          Stream::Error stream;
          stream << FNS
                 << "Logic error. Not existing client_id="
                 << client_ids_[position];
          logger_->error(stream.str(), Aspect::CLIENT_POOL_CORO);
          return client;
        }
        it_change->second.second = position;
      }

      return client;
    }
    catch (const eh::Exception &exc)
    {
      try
      {
        Stream::Error stream;
        stream << FNS
               << ": "
               << exc.what();
        logger_->error(stream.str(), Aspect::CLIENT_POOL_CORO);
      }
      catch (...)
      {
      }
    }
    catch (...)
    {
      try
      {
        Stream::Error stream;
        stream << FNS
               << ": Unknown error";
        logger_->error(stream.str(), Aspect::CLIENT_POOL_CORO);
      }
      catch (...)
      {
      }
    }

    return {};
  }

private:
  const Logger_var logger_;

  Clients clients_;

  ClientIds client_ids_;

  mutable Mutex mutex_client_;

  Counter counter_{0};
};

} // namespace Internal

template<class RpcServiceMethodConcept>
class ClientPoolCoro final :
  public std::enable_shared_from_this<
    ClientPoolCoro<RpcServiceMethodConcept>>
{
private:
  using Impl = Internal::ClientPoolCoroImpl<RpcServiceMethodConcept>;
  using ImplPtr = std::unique_ptr<Impl>;
  using FactoryPtr = std::unique_ptr<Factory<RpcServiceMethodConcept>>;
  using Client = ClientCoro<RpcServiceMethodConcept>;

public:
  using Channels = typename Factory<RpcServiceMethodConcept>::Channels;
  using SchedulerPtr = typename Factory<RpcServiceMethodConcept>::SchedulerPtr;
  using RequestPtr = typename Impl::RequestPtr;
  using ClientPoolCoroPtr = std::shared_ptr<ClientPoolCoro>;
  using TaskProcessor = userver::engine::TaskProcessor;
  using TaskProcessorRef = std::reference_wrapper<TaskProcessor>;
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using WriteResult = typename Impl::WriteResult;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  ~ClientPoolCoro() = default;

  static ClientPoolCoroPtr create(
    Logger* logger,
    const SchedulerPtr& scheduler,
    const Channels& channels,
    const std::size_t number_async_client,
    TaskProcessor& task_processor)
  {
    ClientPoolCoroPtr pool(new ClientPoolCoro(
      logger,
      scheduler,
      channels,
      number_async_client,
      task_processor));
    pool->initialize();

    return pool;
  }

  static ClientPoolCoroPtr create(
    Logger* logger,
    const SchedulerPtr& scheduler,
    const Channels& channels,
    const std::size_t number_async_client)
  {
    ClientPoolCoroPtr pool(new ClientPoolCoro(
      logger,
      scheduler,
      channels,
      number_async_client));
    pool->initialize();

    return pool;
  }

  WriteResult write(
    RequestPtr&& request,
    const std::size_t timeout = 3000)  noexcept
  {
    try
    {
      auto result = UServerUtils::Utils::run_in_coro(
        task_processor_,
        UServerUtils::Utils::Importance::kNormal,
        {},
        [ptr = this->shared_from_this(),
         request = std::move(request),
         timeout] () mutable {
          auto& impl = ptr->impl_;
          return impl->write(std::move(request), timeout);
        }
      );

      return result;
    }
    catch (const eh::Exception& exc)
    {
      try
      {
        Stream::Error stream;
        stream << FNS
               << ": "
               << exc.what();
        logger_->error(stream.str(), Aspect::CLIENT_POOL_CORO);
      }
      catch (...)
      {
      }
    }
    catch (...)
    {
      try
      {
        Stream::Error stream;
        stream << FNS
               << ": Unknown error";
        logger_->error(stream.str(), Aspect::CLIENT_POOL_CORO);
      }
      catch (...)
      {
      }
    }

    return WriteResult(Status::InternalError, {});
  }

  bool ok(const std::optional<std::size_t> max_number_check_channels = {}) const noexcept
  {
    std::size_t count = channels_.size();
    if (max_number_check_channels)
    {
      const std::size_t count = std::min(
        count,
        *max_number_check_channels);
    }

    for (std::size_t i = 0; i < count; ++i)
    {
      const auto& channel = channels_[i];
      const auto state = channel->GetState(false);
      const bool is_bad =
        state == GRPC_CHANNEL_TRANSIENT_FAILURE || state == GRPC_CHANNEL_SHUTDOWN;
      if (!is_bad)
      {
        return true;
      }
    }

    return false;
  }

private:
  explicit ClientPoolCoro(
    Logger* logger,
    const SchedulerPtr& scheduler,
    const Channels& channels,
    const std::size_t number_async_client,
    TaskProcessor& task_processor)
    : logger_(ReferenceCounting::add_ref(logger)),
      scheduler_(scheduler),
      channels_(channels),
      number_async_client_(number_async_client),
      task_processor_(task_processor)
  {
    if (channels.empty())
    {
      Stream::Error stream;
      stream << FNS
             << "Number channels is nil";
      throw Exception(stream);
    }
  }

  explicit ClientPoolCoro(
    Logger* logger,
    const SchedulerPtr& scheduler,
    const Channels& channels,
    const std::size_t number_async_client)
    : logger_(ReferenceCounting::add_ref(logger)),
      scheduler_(scheduler),
      channels_(channels),
      number_async_client_(number_async_client)
  {
    if (channels.empty())
    {
      Stream::Error stream;
      stream << FNS
             << "Number channels is nil";
      throw Exception(stream);
    }

    const bool is_task_processor_thread =
      userver::engine::current_task::IsTaskProcessorThread();
    if (!is_task_processor_thread)
    {
      Stream::Error stream;
      stream << FNS
             << "ClientPoolCoro must be call on coroutine pool";
      throw Exception(stream);
    }

    auto& current_task_processor =
      userver::engine::current_task::GetTaskProcessor();
    task_processor_ = current_task_processor;
  }

  void initialize()
  {
    using Channel = grpc::Channel;
    using ChannelPtr = std::shared_ptr<Channel>;
    using CompletionQueue = grpc::CompletionQueue;
    using CompletionQueuePtr = std::shared_ptr<CompletionQueue>;

    UServerUtils::Utils::run_in_coro(
      task_processor_,
      UServerUtils::Utils::Importance::kCritical,
      {},
      [ptr = this->shared_from_this()] () {
        auto& factory = ptr->factory_;
        auto& impl = ptr->impl_;
        auto& scheduler = ptr->scheduler_;
        auto& channels = ptr->channels_;
        auto logger = ptr->logger_;
        const auto number_async_client = ptr->number_async_client_;
        auto& task_processor = userver::engine::current_task::GetTaskProcessor();

        auto weak_ptr = std::weak_ptr<ClientPoolCoro>(ptr);
        auto factory_observer = [weak_ptr, task_processor = &task_processor, logger] (
          const ClientId client_id,
          const ChannelPtr& channel,
          const CompletionQueuePtr& completion_queue) mutable {
            try
            {
              userver::engine::CriticalAsyncNoSpan(
                *task_processor,
                [
                  client_id,
                  weak_ptr,
                  logger,
                  channel = std::weak_ptr<Channel>(channel),
                  completion_queue = std::weak_ptr<CompletionQueue>(completion_queue)] () mutable {

                  if (auto ptr = weak_ptr.lock())
                  {
                    auto& impl = ptr->impl_;
                    auto client = impl->remove(client_id);
                    if (!client)
                    {
                      Stream::Error stream;
                      stream << FNS
                             << "Logic error... Client is null";
                      logger->error(stream.str(), Aspect::CLIENT_POOL_CORO);
                      return;
                    }
                  }
                  else
                  {
                    return;
                  }

                  for (;;)
                  {
                    auto channel_ptr = channel.lock();
                    auto completion_queue_ptr = completion_queue.lock();
                    auto ptr = weak_ptr.lock();

                    if (!ptr || !channel_ptr || !completion_queue_ptr)
                      return;

                    auto state = channel_ptr->GetState(false);
                    if (state == GRPC_CHANNEL_READY || state == GRPC_CHANNEL_IDLE)
                    {
                      auto& impl = ptr->impl_;
                      auto& factory = ptr->factory_;
                      auto client = Client::create(logger);
                      factory->create(client, channel_ptr);
                      impl->emplace(std::move(client));
                      break;
                    }
                    else
                    {
                      userver::engine::Promise<void> promise;
                      auto future = promise.get_future();
                      auto event = std::make_unique<EventChannelState>(std::move(promise));
                      channel_ptr->NotifyOnStateChange(
                        state,
                        gpr_inf_future(GPR_CLOCK_MONOTONIC),
                        completion_queue_ptr.get(),
                        event.release());

                      channel_ptr.reset();
                      completion_queue_ptr.reset();
                      ptr.reset();

                      future.get();
                    }
                  }
                }
              ).Detach();
            }
            catch (const eh::Exception& exc)
            {
              try
              {
                Stream::Error stream;
                stream << FNS
                       << exc.what();
                logger->error(stream.str(), Aspect::CLIENT_POOL_CORO);
              }
              catch (...)
              {
              }
            }
            catch (...)
            {
              try
              {
                Stream::Error stream;
                stream << FNS
                       << "Unknown error";
                logger->error(stream.str(), Aspect::CLIENT_POOL_CORO);
              }
              catch (...)
              {
              }
            }
         };

        factory = std::make_unique<Factory<RpcServiceMethodConcept>>(
          logger,
          scheduler,
          channels,
          std::move(factory_observer));
        impl = std::make_unique<Impl>(logger);

        const auto number_thread = factory->number_thread();
        if (number_thread == 0)
        {
          Stream::Error stream;
          stream << FNS
                 << ": number thread of factory is null";
          logger->error(stream.str(), Aspect::CLIENT_POOL_CORO);
          throw Exception(stream);
        }

        for (std::size_t i = 0; i < number_async_client; ++i)
        {
          auto client = Client::create(logger);
          factory->create(client);
          impl->emplace(std::move(client));
        }
      }
    );
  }

private:
  const Logger_var logger_;

  const SchedulerPtr scheduler_;

  const Channels channels_;

  const std::size_t number_async_client_;

  TaskProcessorRef task_processor_;

  ImplPtr impl_;

  FactoryPtr factory_;
};

template<class RpcServiceMethodConcept>
using ClientPoolCoroPtr = std::unique_ptr<ClientPoolCoro<RpcServiceMethodConcept>>;

} // namespace UServerUtils::Grpc::Client

#endif // GRPC_CLIENT_CLIENT_POOL_CORO_H_
