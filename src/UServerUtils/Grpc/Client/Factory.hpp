#ifndef GRPC_CLIENT_FACTORY_H_
#define GRPC_CLIENT_FACTORY_H_

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
#include <UServerUtils/Grpc/Client/Config.hpp>
#include <UServerUtils/Grpc/Client/ClientImpl.hpp>
#include <UServerUtils/Grpc/Client/FactoryObserver.hpp>
#include <UServerUtils/Grpc/Client/Writer.hpp>
#include <UServerUtils/Grpc/Client/Types.hpp>
#include <UServerUtils/Grpc/Common/QueueAtomic.hpp>
#include <UServerUtils/Grpc/Common/RpcServiceMethodTraits.hpp>
#include <UServerUtils/Grpc/Common/Scheduler.hpp>
#include <UServerUtils/Grpc/Common/Utils.hpp>

namespace UServerUtils::Grpc::Client
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
    const std::size_t adding = (*number_channels % number_threads != 0);
    *number_channels = (*number_channels / number_threads + adding) * number_threads;
  }

  grpc::ChannelArguments channel_arguments;
  for (const auto& [key, value] : channel_map_args)
  {
    if (UServerUtils::Grpc::Common::Utils::is_integer(value))
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

  static constexpr auto k_rpc_type = Traits<RpcServiceMethodConcept>::rpc_type;

  using WriterPtr = std::unique_ptr<Writer<Request, k_rpc_type>>;
  using Observer = ClientObserver<RpcServiceMethodConcept>;
  using ObserverPtr = std::shared_ptr<Observer>;
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using Channel = grpc::Channel;
  using ChannelPtr = std::shared_ptr<Channel>;
  using Channels = std::vector<ChannelPtr>;
  using SchedulerPtr = Common::SchedulerPtr;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

private:
  using SchedulerQueue = typename Common::Scheduler::Queue;
  using SchedulerQueues = typename Common::Scheduler::Queues;
  using CompletionQueue = grpc::CompletionQueue;
  using CompletionQueuePtr = std::shared_ptr<CompletionQueue>;
  using ClientPtr = std::shared_ptr<Client<Request>>;

  struct ChannelData final
  {
    ChannelPtr channel;
    CompletionQueuePtr completion_queue;
  };
  using ChannelsData = std::vector<ChannelData>;
  using Index = std::size_t;
  using ChannelId = std::uintptr_t;
  using ChannelIdToIndex = std::unordered_map<ChannelId, Index>;

  struct ClientInfo final
  {
    ClientInfo(
      const ClientPtr& client,
      const ObserverPtr& observer,
      const Index index)
      : client(client),
        observer(observer),
        index(index)
    {
    }

    ClientInfo(const ClientInfo&) = delete;
    ClientInfo(ClientInfo&&) = default;
    ClientInfo& operator=(const ClientInfo&) = delete;
    ClientInfo& operator=(ClientInfo&&) = default;

    ClientPtr client;
    ObserverPtr observer;
    Index index = 0;
  };
  using Clients = std::unordered_map<ClientId, ClientInfo>;

public:
  explicit FactoryImpl(
    const Config& config,
    Logger* logger,
    FactoryObserver&& factory_observer = {})
    : logger_(ReferenceCounting::add_ref(logger)),
      factory_observer_(std::move(factory_observer))
  {
    scheduler_ = UServerUtils::Grpc::Common::Utils::create_scheduler(
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
      auto& channel = channels[i];
      const ChannelId channel_id = reinterpret_cast<std::uintptr_t>(channel.get());

      ChannelData channel_data;
      channel_data.completion_queue = scheduler_queues[i % scheduler_queues.size()];
      channel_data.channel = std::move(channel);
      channels_data_.emplace_back(std::move(channel_data));

      channel_id_to_index_.emplace(
        channel_id,
        channels_data_.size() - 1);
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
      auto& channel = channels[i];
      const ChannelId channel_id = reinterpret_cast<std::uintptr_t>(channel.get());

      ChannelData channel_data;
      channel_data.completion_queue = scheduler_queues[i % scheduler_queues.size()];
      channel_data.channel = std::move(channel);
      channels_data_.emplace_back(std::move(channel_data));

      channel_id_to_index_.emplace(
        channel_id,
        channels_data_.size() - 1);
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
        {
          return;
        }
      }

      const auto status = cv_.wait_for(
        lock,
        std::chrono::milliseconds(5000),
        [this] () {
          return clients_.empty();
        });
      if (!status)
      {
        Stream::Error stream;
        stream << FNS
               << "Logic error. Timout is reached.";
        logger_->error(stream.str(), Aspect::FACTORY);
      }
    }
    catch (const eh::Exception& exc)
    {
      Stream::Error stream;
      stream << FNS
             << exc.what();
      logger_->error(stream.str(), Aspect::FACTORY);
    }
  }

  ~FactoryImpl() = default;

  WriterPtr create(
    const ObserverPtr& observer,
    RequestPtr&& request,
    const std::optional<ChannelPtr>& channel)
  {
    if constexpr (k_rpc_type == grpc::internal::RpcMethod::NORMAL_RPC
      || k_rpc_type == grpc::internal::RpcMethod::SERVER_STREAMING)
    {
      if (!request)
      {
        Stream::Error stream;
        stream << FNS
               << "Request is null";
        throw Exception(stream);
      }
    }

    Index index = 0;
    if (channel.has_value())
    {
      if (!*channel)
      {
        Stream::Error stream;
        stream << FNS
               << "Channel is null";
        throw Exception(stream);
      }

      const auto it = channel_id_to_index_.find(
        reinterpret_cast<ChannelId>(channel->get()));
      if (it == channel_id_to_index_.end())
      {
        Stream::Error stream;
        stream << FNS
               << "Logic error... Not existing channel";
        logger_->error(stream.str(), Aspect::FACTORY);

        const auto number = counter_.fetch_add(
          1,
          std::memory_order_relaxed);
        index = number % channels_data_.size();
      }
      else
      {
        index = it->second;
      }
    }
    else
    {
      const auto number = counter_.fetch_add(
        1,
        std::memory_order_relaxed);
      index = number % channels_data_.size();
    }

    auto& channel_data = channels_data_[index];
    auto client = ClientImpl<RpcServiceMethodConcept>::create(
      logger_,
      channel_data.channel,
      channel_data.completion_queue,
      observer,
      *this,
      std::move(request));

    if constexpr (k_rpc_type == grpc::internal::RpcMethod::CLIENT_STREAMING
      || k_rpc_type == grpc::internal::RpcMethod::BIDI_STREAMING)
    {
      auto writer = std::make_shared<Writer<Request, k_rpc_type>>(client);
      observer->set_writer(writer);
      observer->set_completion_queue(channel_data.completion_queue);
    }

    {
      std::lock_guard lock(mutex_clients_);
      auto result = clients_.try_emplace(
        client->get_id(),
        client,
        observer,
        index);
      if (!result.second)
      {
        Stream::Error stream;
        stream << FNS
               << "Logic error. Client with id="
               << client->get_id()
               << " already exist";
        throw Exception(stream);
      }
    }

    client->start();

    return std::make_unique<Writer<Request, k_rpc_type>>(client);
  }

  std::size_t size() const noexcept
  {
    std::lock_guard lock(mutex_clients_);
    return clients_.size();
  }

  std::size_t number_thread() const noexcept
  {
    return scheduler_->size();
  }

private:
  void need_remove(const ClientId client_id) noexcept override
  {
    ChannelPtr channel;
    CompletionQueuePtr completion_queue;
    {
      std::lock_guard lock(mutex_clients_);
      auto it = clients_.find(client_id);
      if (it == clients_.end())
      {
        return;
      }

      const auto& client_info = it->second;
      const auto index = client_info.index;
      const auto& channel_data = channels_data_[index];
      channel = channel_data.channel;
      completion_queue = channel_data.completion_queue;
    }

    if (factory_observer_)
    {
      factory_observer_(
        client_id,
        channel,
        completion_queue);
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

  SchedulerPtr scheduler_;

  FactoryObserver factory_observer_;

  ChannelsData channels_data_;

  ChannelIdToIndex channel_id_to_index_;

  std::atomic<std::size_t> counter_{0};

  Clients clients_;

  bool is_stopped_ = false;

  std::condition_variable cv_;

  mutable std::mutex mutex_clients_;
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
  using ObserverPtr = typename Impl::ObserverPtr;
  using Request = typename Impl::Request;
  using RequestPtr = typename Impl::RequestPtr;
  using Response = typename Impl::Response;

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

  void create(
    const ObserverPtr& observer,
    RequestPtr&& request)
  {
    impl_.create(
      observer,
      std::move(request),
      {});
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
  using ChannelPtr = typename Impl::ChannelPtr;

public:
  using Logger = Logging::Logger;
  using Channels = typename Impl::Channels;
  using SchedulerPtr = typename Impl::SchedulerPtr;
  using Observer = typename Impl::Observer;
  using ObserverPtr = typename Impl::ObserverPtr;
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

  WriterPtr create(
    const ObserverPtr& observer,
    const std::optional<ChannelPtr>& channel = {})
  {
    return impl_.create(observer, {}, channel);
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
  using ChannelPtr = typename Impl::ChannelPtr;
  using Channels = typename Impl::Channels;
  using SchedulerPtr = typename Impl::SchedulerPtr;
  using ObserverPtr = typename Impl::ObserverPtr;
  using Request = typename Impl::Request;
  using RequestPtr = typename Impl::RequestPtr;
  using Response = typename Impl::Response;

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

  void create(
    const ObserverPtr& observer,
    RequestPtr&& request,
    const std::optional<ChannelPtr>& channel = {})
  {
    impl_.create(observer, std::move(request), channel);
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
  using ChannelPtr = typename Impl::ChannelPtr;
  using Channels = typename Impl::Channels;
  using SchedulerPtr = typename Impl::SchedulerPtr;
  using ObserverPtr = typename Impl::ObserverPtr;
  using WriterPtr = typename Impl::WriterPtr;
  using Request = typename Impl::Request;
  using RequestPtr = typename Impl::RequestPtr;
  using Response = typename Impl::Response;

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

  WriterPtr create(
    const ObserverPtr& observer,
    const std::optional<ChannelPtr>& channel = {})
  {
    return impl_.create(observer, {}, channel);
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

} // namespace UServerUtils::Grpc::Client

#endif // GRPC_CLIENT_FACTORY_H_