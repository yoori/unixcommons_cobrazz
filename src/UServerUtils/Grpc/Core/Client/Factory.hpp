#ifndef GRPC_CORE_CLIENT_FACTORY_H_
#define GRPC_CORE_CLIENT_FACTORY_H_

// STD
#include <cassert>
#include <memory>
#include <queue>
#include <unordered_map>
#include <thread>

// GRPC
#include <grpcpp/grpcpp.h>

// THIS
#include "Config.hpp"
#include "ClientImpl.hpp"
#include "FactoryObserver.hpp"
#include "Writer.hpp"
#include "Types.hpp"
#include "Common/QueueAtomic.hpp"
#include "Common/Scheduler.hpp"
#include "Common/RpcServiceMethodTraits.hpp"
#include "Generics/Uncopyable.hpp"

namespace UServerUtils::Grpc::Core::Client
{

namespace Aspect
{

constexpr const char FACTORY[] = "FACTORY";

} // namespace Aspect

namespace Internal
{

template<class RpcServiceMethodConcept>
class FactoryImpl final
  : public ClientDelegate<typename Traits<RpcServiceMethodConcept>::Request>
{
public:
  using Request = typename Traits<RpcServiceMethodConcept>::Request;
  using RequestPtr = std::unique_ptr<Request>;
  using Response = typename Traits<RpcServiceMethodConcept>::Response;
  using ResponsePtr = std::unique_ptr<Response>;

  static constexpr auto k_rpc_type =
    Traits<RpcServiceMethodConcept>::rpc_type;

  using WriterPtr = std::unique_ptr<Writer<Request, k_rpc_type>>;
  using Observer = ClientObserver<RpcServiceMethodConcept>;
  using Logger_var = Logging::Logger_var;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

private:
  using SchedulerQueue = typename Common::Scheduler::Queue;
  using SchedulerQueues = typename Common::Scheduler::Queues;

  struct ChannelData final
  {
    using ChannelPtr = std::shared_ptr<grpc::Channel>;
    using CompletionQueue = grpc::CompletionQueue;
    using CompletionQueuePtr = std::shared_ptr<CompletionQueue>;

    ChannelData() = default;
    ~ChannelData() = default;

    ChannelPtr channel;
    CompletionQueuePtr completion_queue;
  };
  using ChannelsData = std::vector<ChannelData>;
  using IndexChannelData = std::size_t;

  struct ClientInfo final
  {
    using ClientPtr = std::shared_ptr<Client<Request>>;
    using ObserverRef = std::reference_wrapper<Observer>;

    ClientInfo(
      const ClientPtr& client,
      const IndexChannelData index_channel_data,
      Observer& observer)
      : client(client),
        index_channel_data(index_channel_data),
        observer(observer)
    {
    }

    ~ClientInfo() = default;

    ClientPtr client;
    IndexChannelData index_channel_data = 0;
    ObserverRef observer;
  };
  using Clients = std::unordered_map<ClientId, ClientInfo>;

  using QueueFinishedIndex = Common::QueueAtomic<IndexChannelData>;

public:
  explicit FactoryImpl(
    const Config& config,
    const Logger_var& logger,
    FactoryObserver&& factory_observer = {})
    : logger_(logger),
      factory_observer_(std::move(factory_observer))
  {
    auto number_threads = config.number_threads;
    if (!number_threads)
    {
      const auto best_thread_number =
        std::thread::hardware_concurrency();
      if (best_thread_number == 0)
      {
        Stream::Error stream;
        stream << FNS
               << ": hardware_concurrency is failed";
        logger_->error(stream.str(), Aspect::FACTORY);
      }
      number_threads =
        best_thread_number ? best_thread_number : 8;
    }

    auto number_channels = config.number_channels;
    if (!number_channels)
    {
      number_channels = number_threads;
    }
    else
    {
      const std::size_t adding =
        (*number_channels % *number_threads != 0);
      *number_channels =
        (*number_channels / *number_threads + adding) * *number_threads;
    }

    SchedulerQueues scheduler_queues;
    scheduler_queues.reserve(*number_threads);
    for (std::size_t i = 1; i <= *number_threads; ++i)
    {
      auto completion_queue = std::make_shared<grpc::CompletionQueue>();
      scheduler_queues.emplace_back(std::move(completion_queue));
    }

    channels_data_.reserve(*number_channels);
    for (std::size_t i = 1; i <= *number_channels; ++i)
    {
      auto channel_args = config.channel_args;
      channel_args.SetString(
        "key_for_unique_tcp",
        std::to_string(i) + std::to_string(std::uintptr_t(this)));
      auto channel = grpc::CreateCustomChannel(
        config.endpoint,
        config.credentials,
        channel_args);

      ChannelData channel_data;
      channel_data.completion_queue = scheduler_queues[i % scheduler_queues.size()];
      channel_data.channel = std::move(channel);
      channels_data_.emplace_back(std::move(channel_data));
    }

    scheduler_ = Common::Scheduler_var(
      new Common::Scheduler(
        logger_,
        std::move(scheduler_queues)));
    scheduler_->activate_object();
  }

  void stop() noexcept
  {
    try
    {
      std::deque<std::future<void>> futures;
      {
        std::lock_guard lock(mutex_clients_);
        for (auto& client : clients_)
        {
          std::promise<void> promise;
          auto future = promise.get_future();
          if (client.second.client->stop(std::move(promise)))
          {
            futures.emplace_back(std::move(future));
          }
        }
      }

      for (auto& future : futures)
      {
        future.wait();
      }

      scheduler_->deactivate_object();
      scheduler_->wait_object();
    }
    catch (const eh::Exception& exc)
    {
      try
      {
        Stream::Error stream;
        stream << FNS
               << ": "
               << exc.what();
        logger_->error(stream.str(), Aspect::FACTORY);
      }
      catch (...)
      {
      }
    }
  }

  ~FactoryImpl() = default;

  WriterPtr create(Observer& observer, RequestPtr&& request)
  {
    if constexpr (k_rpc_type == grpc::internal::RpcMethod::NORMAL_RPC
               || k_rpc_type == grpc::internal::RpcMethod::SERVER_STREAMING)
    {
      if (!request)
      {
        Stream::Error stream;
        stream << FNS
               << ": Request is null";
        throw Exception(stream);
      }
    }

    IndexChannelData index_channel_data = 0;
    std::unique_ptr<IndexChannelData> data;
    if constexpr (k_rpc_type != grpc::internal::RpcMethod::NORMAL_RPC)
    {
      data = queue_finished_index_.pop();
    }

    if (data)
    {
      index_channel_data = *data;
    }
    else
    {
      const auto number = counter_.fetch_add(
        1,
        std::memory_order_relaxed);
      index_channel_data = number % channels_data_.size();
    }
    auto& channel_data = channels_data_[index_channel_data];

    auto client = ClientImpl<RpcServiceMethodConcept>::create(
      logger_,
      channel_data.channel,
      channel_data.completion_queue,
      *this,
      observer,
      std::move(request));

    auto writer = std::make_unique<Writer<Request, k_rpc_type>>(
      channel_data.completion_queue,
      client,
      client->get_id());

    {
      std::lock_guard lock(mutex_clients_);
      auto result = clients_.try_emplace(
        client->get_id(),
        client,
        index_channel_data,
        observer);
      if (!result.second)
      {
        Stream::Error stream;
        stream << FNS
               << ": "
               << ": Logic error. Client with id="
               << client->get_id()
               << " already exist";
        throw Exception(stream);
      }
    }

    client->start();

    return writer;
  }

  std::size_t size() const noexcept
  {
    try
    {
      std::lock_guard lock(mutex_clients_);
      return clients_.size();
    }
    catch (...)
    {
      return 0;
    }
  }

  std::size_t number_thread() const noexcept
  {
    return scheduler_->size();
  }

private:
  void need_remove(const ClientId client_id) noexcept
  {
    IndexChannelData index_channel_data = 0;
    {
      std::lock_guard lock(mutex_clients_);
      auto it = clients_.find(client_id);
      if (it == clients_.end())
        return;

      if constexpr (k_rpc_type != Internal::RpcType::NORMAL_RPC)
      {
        index_channel_data = it->second.index_channel_data;
      }
      clients_.erase(it);
    }

    if constexpr (k_rpc_type != Internal::RpcType::NORMAL_RPC)
    {
      try
      {
        queue_finished_index_.emplace(index_channel_data);
      }
      catch (...)
      {
      }
    }

    if (factory_observer_)
    {
      factory_observer_(client_id);
    }
  }

private:
  Logger_var logger_;

  FactoryObserver factory_observer_;

  Common::Scheduler_var scheduler_;

  ChannelsData channels_data_;

  std::atomic<std::size_t> counter_{0};

  Clients clients_;

  mutable std::mutex mutex_clients_;

  QueueFinishedIndex queue_finished_index_;

  bool is_stopped = true;
};

} // namespace Internal

template<class RpcServiceMethodConcept, class = void>
class Factory
{
};

template<class RpcServiceMethodConcept>
class Factory<
  RpcServiceMethodConcept,
  Internal::has_normal_rpc_t<RpcServiceMethodConcept>> final
  : protected Generics::Uncopyable
{
private:
  using Impl = Internal::FactoryImpl<RpcServiceMethodConcept>;

public:
  using Logger_var = Logging::Logger_var;
  using Observer = typename Impl::Observer;
  using Request = typename Impl::Request;
  using RequestPtr = typename Impl::RequestPtr;
  using Response = typename Impl::Response;
  using ResponsePtr = typename Impl::ResponsePtr;

public:
  explicit Factory(
    const Config& config,
    const Logger_var& logger,
    FactoryObserver&& factory_observer = {})
    : impl_(config, logger, std::move(factory_observer))
  {
  }

  ~Factory()
  {
    impl_.stop();
  }

  void create(Observer& observer, RequestPtr&& request)
  {
    impl_.create(observer, std::move(request));
  }

  std::size_t size() const noexcept
  {
    return impl_.size();
  }

  std::size_t number_thread() const noexcept
  {
    return impl_.number_thread();
  }

private:
  Impl impl_;
};

template<class RpcServiceMethodConcept>
class Factory<
  RpcServiceMethodConcept,
  Internal::has_client_streaming_t<RpcServiceMethodConcept>>
  : protected Generics::Uncopyable
{
private:
  using Impl = Internal::FactoryImpl<RpcServiceMethodConcept>;

public:
  using Logger_var = Logging::Logger_var;
  using Observer = typename Impl::Observer;
  using WriterPtr = typename Impl::WriterPtr;
  using Request = typename Impl::Request;
  using RequestPtr = typename Impl::RequestPtr;
  using Response = typename Impl::Response;
  using ResponsePtr = typename Impl::ResponsePtr;

public:
  explicit Factory(
    const Config& config,
    const Logger_var& logger,
    FactoryObserver&& factory_observer = {})
    : impl_(config, logger, std::move(factory_observer))
  {
  }

  ~Factory()
  {
    impl_.stop();
  }

  WriterPtr create(Observer& observer)
  {
    return impl_.create(observer, {});
  }

  std::size_t size() const noexcept
  {
    return impl_.size();
  }

  std::size_t number_thread() const noexcept
  {
    return impl_.number_thread();
  }

private:
  Impl impl_;
};

template<class RpcServiceMethodConcept>
class Factory<
  RpcServiceMethodConcept,
  Internal::has_server_streaming_t<RpcServiceMethodConcept>>
  : protected Generics::Uncopyable
{
private:
  using Impl = Internal::FactoryImpl<RpcServiceMethodConcept>;

public:
  using Logger_var = Logging::Logger_var;
  using Observer = typename Impl::Observer;
  using Request = typename Impl::Request;
  using RequestPtr = typename Impl::RequestPtr;
  using Response = typename Impl::Response;
  using ResponsePtr = typename Impl::ResponsePtr;

public:
  explicit Factory(
    const Config& config,
    const Logger_var& logger,
    FactoryObserver&& factory_observer = {})
    : impl_(config, logger, std::move(factory_observer))
  {
  }

  ~Factory()
  {
    impl_.stop();
  }

  void create(Observer& observer, RequestPtr&& request)
  {
    impl_.create(observer, std::move(request));
  }

  std::size_t size() const noexcept
  {
    return impl_.size();
  }

  std::size_t number_thread() const noexcept
  {
    return impl_.number_thread();
  }

private:
  Impl impl_;
};

template<class RpcServiceMethodConcept>
class Factory<
  RpcServiceMethodConcept,
  Internal::has_bidi_streaming_t<RpcServiceMethodConcept>>
  : protected Generics::Uncopyable
{
private:
  using Impl = Internal::FactoryImpl<RpcServiceMethodConcept>;

public:
  using Logger_var = Logging::Logger_var;
  using Observer = typename Impl::Observer;
  using WriterPtr = typename Impl::WriterPtr;
  using Request = typename Impl::Request;
  using RequestPtr = typename Impl::RequestPtr;
  using Response = typename Impl::Response;
  using ResponsePtr = typename Impl::ResponsePtr;

public:
  explicit Factory(
    const Config& config,
    const Logger_var& logger,
    FactoryObserver&& factory_observer = {})
    : impl_(config, logger, std::move(factory_observer))
  {
  }

  ~Factory()
  {
    impl_.stop();
  }

  WriterPtr create(Observer& observer)
  {
    return impl_.create(observer, {});
  }

  std::size_t size() const noexcept
  {
    return impl_.size();
  }

  std::size_t number_thread() const noexcept
  {
    return impl_.number_thread();
  }

private:
  Impl impl_;
};

} // namespace UServerUtils::Grpc::Core::Client

#endif // GRPC_CORE_CLIENT_FACTORY_H_
