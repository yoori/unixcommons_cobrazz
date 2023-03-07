#ifndef GRPC_CORE_CLIENT_CLIENT_POOL_CORO_H_
#define GRPC_CORE_CLIENT_CLIENT_POOL_CORO_H_

// STD
#include <memory>
#include <unordered_map>

// GRPC
#include <grpcpp/grpcpp.h>

// USERVER
#include <userver/engine/async.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/sleep.hpp>
#include <engine/task/task_processor.hpp>

// THIS
#include "ClientCoro.hpp"
#include "ClientObserver.hpp"
#include "Config.hpp"
#include "ConfigPoolCoro.hpp"
#include "Factory.hpp"
#include "FactoryObserver.hpp"
#include "Types.hpp"
#include <Generics/Uncopyable.hpp>
#include <Logger/Logger.hpp>
#include <UServerUtils/Grpc/Utils.hpp>

namespace UServerUtils::Grpc::Core::Client
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
{
public:
  using Logger_var = Logging::Logger_var;
  using Traits = Internal::Traits<RpcServiceMethodConcept>;
  using Request = typename Traits::Request;
  using RequestPtr = std::unique_ptr<Request>;
  using WriteResult =
    typename ClientCoro<RpcServiceMethodConcept>::WriteResult;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

private:
  using ClientPtr = ClientCoroPtr<RpcServiceMethodConcept>;
  using Position = std::size_t;
  using ClientInfo = std::pair<ClientPtr, Position>;
  using Clients = std::unordered_map<ClientId, ClientInfo>;
  using ClientIds = std::vector<ClientId>;
  using Counter = std::atomic<std::uint32_t>;
  using Mutex = userver::engine::Mutex;

public:
  ClientPoolCoroImpl(const Logger_var& logger)
    : logger_(logger)
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
               << ": request is null";
        throw Exception(stream);
      }

      const auto number = counter_.fetch_add(
        1,
        std::memory_order_relaxed);

      ClientPtr client;
      {
        std::lock_guard lock(mutex_client_);
        const auto size = client_ids_.size();
        if (size == 0)
          return WriteResult(Status::InternalError, {});

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

  void emplace(ClientPtr&& client) noexcept
  {
    try
    {
      const auto client_id = client->client_id();

      std::lock_guard lock(mutex_client_);
      const auto size = client_ids_.size();
      const auto result = clients_.try_emplace(
        client_id,
        std::move(client),
        size);
      if (!result.second)
      {
        Stream::Error stream;
        stream << FNS
               << ": Logic error. Already existing client_id="
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
  }

  void remove(const ClientId client_id) noexcept
  {
    try
    {
      std::lock_guard lock(mutex_client_);
      auto it_remove = clients_.find(client_id);
      if (it_remove == std::end(clients_))
        return;

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
        return;
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
          return;
        }
        it_change->second.second = position;
      }
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
  using RequestPtr = typename Impl::RequestPtr;
  using ClientPoolCoroPtr = std::shared_ptr<ClientPoolCoro>;
  using TaskProcessor = userver::engine::TaskProcessor;
  using TaskProcessorRef = std::reference_wrapper<TaskProcessor>;
  using Logger_var = Logging::Logger_var;
  using WriteResult = typename Impl::WriteResult;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  ~ClientPoolCoro() = default;

  static ClientPoolCoroPtr create(
    const Logger_var& logger,
    const ConfigPoolCoro& config_pool,
    TaskProcessor& task_processor)
  {
    ClientPoolCoroPtr pool(new ClientPoolCoro(
      logger,
      config_pool,
      task_processor));
    pool->initialize();

    return pool;
  }

  static ClientPoolCoroPtr create(
    const Logger_var& logger,
    const ConfigPoolCoro& config_pool)
  {
    ClientPoolCoroPtr pool(new ClientPoolCoro(
      logger,
      config_pool));
    pool->initialize();

    return pool;
  }

  WriteResult write(
    RequestPtr&& request,
    const std::size_t timeout = 3000)  noexcept
  {
    try
    {
      auto result = UServerUtils::Grpc::Utils::run_in_coro(
        task_processor_,
        UServerUtils::Grpc::Utils::Importance::kNormal,
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

private:
  explicit ClientPoolCoro(
    const Logger_var& logger,
    const ConfigPoolCoro& config_pool,
    TaskProcessor& task_processor)
    : logger_(logger),
      config_pool_(config_pool),
      task_processor_(task_processor)
  {
  }

  explicit ClientPoolCoro(
    const Logger_var& logger,
    const ConfigPoolCoro& config_pool)
    : logger_(logger),
      config_pool_(config_pool)
  {
    auto* current_task_processor =
      userver::engine::current_task::GetTaskProcessorOptional();
    if (!current_task_processor)
    {
      Stream::Error stream;
      stream << FNS
             << ": ClientPoolCoro must be call on coroutine pool";
      throw Exception(stream);
    }
    task_processor_ = *current_task_processor;
  }

  void initialize()
  {
     UServerUtils::Grpc::Utils::run_in_coro(
      task_processor_,
      UServerUtils::Grpc::Utils::Importance::kCritical,
      {},
      [ptr = this->shared_from_this()] () {
        auto& config_pool = ptr->config_pool_;

        Config config;
        config.channel_args = config_pool.channel_args;
        config.credentials = config_pool.credentials;
        config.endpoint = config_pool.endpoint;
        config.number_threads = config_pool.number_threads;
        config.number_channels = config_pool.number_channels;

        auto& factory = ptr->factory_;
        auto& impl = ptr->impl_;
        auto logger = ptr->logger_;

        auto* task_processor =
          userver::engine::current_task::GetTaskProcessorOptional();
        if (!task_processor)
          return;

        auto weak_ptr = std::weak_ptr<ClientPoolCoro>(ptr);
        auto factory_observer =
          [weak_ptr, task_processor, logger] (
            const ClientId client_id) mutable {
            try
            {
              userver::engine::AsyncNoSpan(
                *task_processor,
                [client_id, weak_ptr, logger] () mutable {
                  //<--! TODO - exponential time growth
                  userver::engine::SleepFor(
                    std::chrono::milliseconds(1000));
                  if (auto ptr = weak_ptr.lock())
                  {
                    auto& impl = ptr->impl_;
                    auto& factory = ptr->factory_;

                    impl->remove(client_id);
                    auto client = Client::create(logger);
                    factory->create(*client);
                    impl->emplace(std::move(client));
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
                       << ": "
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
                       << ": Unknown error";
                logger->error(stream.str(), Aspect::CLIENT_POOL_CORO);
              }
              catch (...)
              {
              }
            }
         };

        factory = std::make_unique<Factory<RpcServiceMethodConcept>>(
          config,
          logger,
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

        const std::size_t adding =
          config_pool.number_async_client % number_thread != 0;
        const auto number_client =
          (adding + config_pool.number_async_client / number_thread) * number_thread;

        for (std::size_t i = 0; i < number_client; ++i)
        {
          auto client = Client::create(logger);
          factory->create(*client);
          impl->emplace(std::move(client));
        }
      }
    );
  }

private:
  const Logger_var logger_;

  const ConfigPoolCoro config_pool_;

  TaskProcessorRef task_processor_;

  ImplPtr impl_;

  FactoryPtr factory_;
};

template<class RpcServiceMethodConcept>
using ClientPoolCoroPtr = std::unique_ptr<ClientPoolCoro<RpcServiceMethodConcept>>;

} // namespace UServerUtils::Grpc::Core::Client

#endif // GRPC_CORE_CLIENT_CLIENT_POOL_CORO_H_