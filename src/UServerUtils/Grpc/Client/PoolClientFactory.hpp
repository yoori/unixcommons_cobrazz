#ifndef GRPC_CLIENT_POOLCLIENTFACTORY_H_
#define GRPC_CLIENT_POOLCLIENTFACTORY_H_

// STD
#include <memory>

// USERVER
#include <userver/engine/task/task_processor.hpp>

// THIS
#include <eh/Exception.hpp>
#include <Logger/Logger.hpp>
#include <UServerUtils/Grpc/Client/ConfigPoolCoro.hpp>
#include <UServerUtils/Grpc/Client/Factory.hpp>
#include <UServerUtils/Grpc/Common/Utils.hpp>

namespace UServerUtils::Grpc::Client
{

class PoolClientFactory final
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using TaskProcessor = userver::engine::TaskProcessor;
  using ConfigPoolCoro = UServerUtils::Grpc::Client::ConfigPoolCoro;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

private:
  using ChannelPtr = std::shared_ptr<grpc::Channel>;
  using Channels = std::vector<ChannelPtr>;
  using SchedulerPtr = UServerUtils::Grpc::Common::SchedulerPtr;

public:
  PoolClientFactory(
    Logger* logger,
    const ConfigPoolCoro& config_pool)
    : logger_(ReferenceCounting::add_ref(logger))
  {
    scheduler_ = UServerUtils::Grpc::Common::Utils::create_scheduler(
      config_pool.number_threads,
      logger_.in());
    const auto number_thread = scheduler_->size();
    initialize(config_pool, number_thread);
  }

  PoolClientFactory(
    Logger* logger,
    const SchedulerPtr& scheduler,
    const ConfigPoolCoro& config_pool)
    : logger_(ReferenceCounting::add_ref(logger)),
      scheduler_(scheduler)
  {
    const auto number_thread = scheduler_->size();
    initialize(config_pool, number_thread);
  }

  ~PoolClientFactory() = default;

  // You must ensure that ClientPool is destroyed
  // before call Manager::deactivate_object
  template<class ClientPool>
  std::shared_ptr<ClientPool> create(
    TaskProcessor& task_processor)
  {
    return ClientPool::create(
      logger_.in(),
      scheduler_,
      channels_,
      number_client_,
      task_processor);
  }

  template<class ClientPool>
  std::shared_ptr<ClientPool> create()
  {
    const bool is_task_processor_thread =
      userver::engine::current_task::IsTaskProcessorThread();
    if (!is_task_processor_thread)
    {
      Stream::Error stream;
      stream << FNS
             << "PoolClientFactory::create"
                " must be call from coroutine pool";
      throw Exception(stream);
    }

    auto& current_task_processor =
      userver::engine::current_task::GetTaskProcessor();

    return ClientPool::create(
      logger_.in(),
      scheduler_,
      channels_,
      number_client_,
      current_task_processor);
  }

private:
  void initialize(
    const ConfigPoolCoro& config_pool,
    const std::size_t number_thread)
  {
    channels_ = UServerUtils::Grpc::Client::Internal::create_channels(
      number_thread,
      config_pool.credentials,
      config_pool.endpoint,
      config_pool.channel_args,
      config_pool.number_channels);

    const auto& number_async_client = config_pool.number_async_client;
    const std::size_t adding = number_async_client % number_thread != 0;
    number_client_ = (adding + number_async_client / number_thread) * number_thread;
  }

private:
  Logger_var logger_;

  SchedulerPtr scheduler_;

  Channels channels_;

  std::size_t number_client_ = 0;
};

} // namespace UServerUtils

#endif // GRPC_CLIENT_POOLCLIENTFACTORY_H_