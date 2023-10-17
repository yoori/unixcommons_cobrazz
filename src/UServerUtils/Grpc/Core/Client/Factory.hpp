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
#include <Generics/Uncopyable.hpp>
#include <UServerUtils/Grpc/Core/Client/Config.hpp>
#include <UServerUtils/Grpc/Core/Client/ClientImpl.hpp>
#include <UServerUtils/Grpc/Core/Client/FactoryObserver.hpp>
#include <UServerUtils/Grpc/Core/Client/Writer.hpp>
#include <UServerUtils/Grpc/Core/Client/Types.hpp>
#include <UServerUtils/Grpc/Core/Common/QueueAtomic.hpp>
#include <UServerUtils/Grpc/Core/Common/RpcServiceMethodTraits.hpp>
#include <UServerUtils/Grpc/Core/Common/Scheduler.hpp>
#include <UServerUtils/Grpc/Core/Common/Utils.hpp>

namespace UServerUtils::Grpc::Core::Client
{

namespace Aspect
{

constexpr const char FACTORY[] = "FACTORY";

} // namespace Aspect

namespace Internal
{

inline auto create_channels(
  const std::size_t number_threads,
  const std::shared_ptr<grpc::ChannelCredentials>& credentials,
  const std::string& endpoint,
  const std::unordered_map<std::string, std::string>& channel_map_args,
  std::optional<std::size_t> number_channels)
{
  using ChannelPtr = std::shared_ptr<grpc::Channel>;
  using Channels = std::vector<ChannelPtr>;

  static std::atomic<std::size_t> counter_{0};

  if (!number_channels)
  {
    number_channels = number_threads;
  }
  else
  {
    const std::size_t adding =
      (*number_channels % number_threads != 0);
    *number_channels =
      (*number_channels / number_threads + adding) * number_threads;
  }

  grpc::ChannelArguments channel_arguments;
  for (const auto& [key, value] : channel_map_args)
  {
    if (UServerUtils::Grpc::Core::Common::Utils::is_integer(value))
    {
      channel_arguments.SetInt(key, std::stoi(value));
    }
    else
    {
      channel_arguments.SetString(key, value);
    }
  }

  Channels channels;
  channels.reserve(*number_channels);
  for (std::size_t i = 1; i <= *number_channels; ++i)
  {
    grpc::ChannelArguments result_channel_arguments(channel_arguments);
    result_channel_arguments.SetString(
      "key_for_unique_tcp",
      std::to_string(counter_.fetch_add(1, std::memory_order_relaxed)));
    auto channel = grpc::CreateCustomChannel(
      endpoint,
      credentials,
      result_channel_arguments);
    channels.emplace_back(std::move(channel));
  }

  return channels;
}

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
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using ChannelPtr = std::shared_ptr<grpc::Channel>;
  using Channels = std::vector<ChannelPtr>;
  using SchedulerPtr = Common::SchedulerPtr;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

private:
  using SchedulerQueue = typename Common::Scheduler::Queue;
  using SchedulerQueues = typename Common::Scheduler::Queues;

  struct ChannelData final
  {
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
    Logger* logger,
    FactoryObserver&& factory_observer = {})
    : logger_(ReferenceCounting::add_ref(logger)),
      factory_observer_(std::move(factory_observer))
  {
    scheduler_ =
      UServerUtils::Grpc::Core::Common::Utils::create_scheduler(
        config.number_threads,
        logger_.in());
    const auto number_threads = scheduler_->size();
    const auto& scheduler_queues = scheduler_->queues();

    auto channels = create_channels(
      number_threads,
      config.credentials,
      config.endpoint,
      config.channel_args,
      config.number_channels);

    const auto size_channels = channels.size();
    channels_data_.reserve(size_channels);
    for (std::size_t i = 0; i < size_channels; ++i)
    {
      ChannelData channel_data;
      channel_data.completion_queue = scheduler_queues[i % scheduler_queues.size()];
      channel_data.channel = std::move(channels[i]);
      channels_data_.emplace_back(std::move(channel_data));
    }
  }

  explicit FactoryImpl(
    Logger* logger,
    const SchedulerPtr& scheduler,
    const Channels& channels,
    FactoryObserver&& factory_observer = {})
    : logger_(ReferenceCounting::add_ref(logger)),
      scheduler_(scheduler),
      factory_observer_(std::move(factory_observer))
  {
    const auto& scheduler_queues = scheduler_->queues();
    const auto size_channels = channels.size();
    channels_data_.reserve(size_channels);
    for (std::size_t i = 0; i < size_channels; ++i)
    {
      ChannelData channel_data;
      channel_data.completion_queue = scheduler_queues[i % scheduler_queues.size()];
      channel_data.channel = channels[i];
      channels_data_.emplace_back(std::move(channel_data));
    }
  }

  void stop() noexcept
  {
    try
    {
      std::unique_lock lock(mutex_clients_);
      is_stopped_ = true;
      for (auto& client : clients_)
      {
        if (!client.second.client->stop())
          return;
      }

      const auto status = cv_.wait_for(
        lock,
        std::chrono::milliseconds(3000),
        [this] () {
          return clients_.empty();
        });
      if (!status)
      {
        Stream::Error stream;
        stream << FNS
               << ": "
               << "Logic error. Timout is reached.";
        logger_->error(stream.str(), Aspect::FACTORY);
      }
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
  void need_remove(const ClientId client_id) noexcept override
  {
    [[maybe_unused]] IndexChannelData index_channel_data = 0;
    {
      std::lock_guard lock(mutex_clients_);
      auto it = clients_.find(client_id);
      if (it == clients_.end())
        return;

      index_channel_data = it->second.index_channel_data;
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

    {
      std::lock_guard lock(mutex_clients_);
      clients_.erase(client_id);

      if (is_stopped_ && clients_.empty())
      {
        cv_.notify_one();
        return;
      }
    }
  }

private:
  const Logger_var logger_;

  Common::SchedulerPtr scheduler_;

  FactoryObserver factory_observer_;

  ChannelsData channels_data_;

  std::atomic<std::size_t> counter_{0};

  Clients clients_;

  bool is_stopped_ = false;

  std::condition_variable cv_;

  mutable std::mutex mutex_clients_;

  QueueFinishedIndex queue_finished_index_{100000};
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
  using Logger = Logging::Logger;
  using Channels = typename Impl::Channels;
  using SchedulerPtr = typename Impl::SchedulerPtr;
  using Observer = typename Impl::Observer;
  using Request = typename Impl::Request;
  using RequestPtr = typename Impl::RequestPtr;
  using Response = typename Impl::Response;
  using ResponsePtr = typename Impl::ResponsePtr;

public:
  explicit Factory(
    const Config& config,
    Logger* logger,
    FactoryObserver&& factory_observer = {})
    : impl_(config, logger, std::move(factory_observer))
  {
  }

  explicit Factory(
    Logger* logger,
    const SchedulerPtr& scheduler,
    const Channels& channels,
    FactoryObserver&& factory_observer = {})
    : impl_(logger, scheduler, channels, std::move(factory_observer))
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
  using Logger = Logging::Logger;
  using Channels = typename Impl::Channels;
  using SchedulerPtr = typename Impl::SchedulerPtr;
  using Observer = typename Impl::Observer;
  using WriterPtr = typename Impl::WriterPtr;
  using Request = typename Impl::Request;
  using RequestPtr = typename Impl::RequestPtr;
  using Response = typename Impl::Response;
  using ResponsePtr = typename Impl::ResponsePtr;

public:
  explicit Factory(
    const Config& config,
    Logger* logger,
    FactoryObserver&& factory_observer = {})
    : impl_(config, logger, std::move(factory_observer))
  {
  }

  explicit Factory(
    Logger* logger,
    const SchedulerPtr& scheduler,
    const Channels& channels,
    FactoryObserver&& factory_observer = {})
    : impl_(logger, scheduler, channels, std::move(factory_observer))
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
  using Logger = Logging::Logger;
  using Channels = typename Impl::Channels;
  using SchedulerPtr = typename Impl::SchedulerPtr;
  using Observer = typename Impl::Observer;
  using Request = typename Impl::Request;
  using RequestPtr = typename Impl::RequestPtr;
  using Response = typename Impl::Response;
  using ResponsePtr = typename Impl::ResponsePtr;

public:
  explicit Factory(
    const Config& config,
    Logger* logger,
    FactoryObserver&& factory_observer = {})
    : impl_(config, logger, std::move(factory_observer))
  {
  }

  explicit Factory(
    Logger* logger,
    const SchedulerPtr& scheduler,
    const Channels& channels,
    FactoryObserver&& factory_observer = {})
    : impl_(logger, scheduler, channels, std::move(factory_observer))
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
  using Logger = Logging::Logger;
  using Channels = typename Impl::Channels;
  using SchedulerPtr = typename Impl::SchedulerPtr;
  using Observer = typename Impl::Observer;
  using WriterPtr = typename Impl::WriterPtr;
  using Request = typename Impl::Request;
  using RequestPtr = typename Impl::RequestPtr;
  using Response = typename Impl::Response;
  using ResponsePtr = typename Impl::ResponsePtr;

public:
  explicit Factory(
    const Config& config,
    Logger* logger,
    FactoryObserver&& factory_observer = {})
    : impl_(config, logger, std::move(factory_observer))
  {
  }

  explicit Factory(
    Logger* logger,
    const SchedulerPtr& scheduler,
    const Channels& channels,
    FactoryObserver&& factory_observer = {})
    : impl_(logger, scheduler, channels, std::move(factory_observer))
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
