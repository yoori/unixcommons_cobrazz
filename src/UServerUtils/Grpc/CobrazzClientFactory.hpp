#ifndef USERVER_GRPC_COBRAZZPOOLCLIENTFACTORY_HPP
#define USERVER_GRPC_COBRAZZPOOLCLIENTFACTORY_HPP

// STD
#include <memory>

// USERVER
#include <engine/task/task_processor.hpp>

// THIS
#include <eh/Exception.hpp>
#include <Logger/Logger.hpp>
#include <UServerUtils/Grpc/Core/Client/Factory.hpp>
#include <UServerUtils/Grpc/Core/Client/ConfigPoolCoro.hpp>
#include <UServerUtils/Grpc/Core/Common/Utils.hpp>

namespace UServerUtils::Grpc
{

class GrpcCobrazzPoolClientFactory final
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using TaskProcessor = userver::engine::TaskProcessor;
  using ConfigPoolCoro = UServerUtils::Grpc::Core::Client::ConfigPoolCoro;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

private:
  using ChannelPtr = std::shared_ptr<grpc::Channel>;
  using Channels = std::vector<ChannelPtr>;
  using SchedulerPtr = UServerUtils::Grpc::Core::Common::SchedulerPtr;

public:
  GrpcCobrazzPoolClientFactory(
    Logger* logger,
    const ConfigPoolCoro& config_pool)
    : logger_(ReferenceCounting::add_ref(logger))
  {
    scheduler_ =
      UServerUtils::Grpc::Core::Common::Utils::create_scheduler(
        config_pool.number_threads,
        logger_.in());
    const auto number_thread = scheduler_->size();
    initialize(config_pool, number_thread);
  }

  GrpcCobrazzPoolClientFactory(
    Logger* logger,
    const SchedulerPtr& scheduler,
    const ConfigPoolCoro& config_pool)
    : logger_(ReferenceCounting::add_ref(logger)),
      scheduler_(scheduler)
  {
    const auto number_thread = scheduler_->size();
    initialize(config_pool, number_thread);
  }

  ~GrpcCobrazzPoolClientFactory() = default;

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
    auto* current_task_processor =
      userver::engine::current_task::GetTaskProcessorOptional();
    if (!current_task_processor)
    {
      Stream::Error stream;
      stream << FNS
             << ": GrpcCobrazzPoolClientFactory::create"
                " must be call from coroutine pool";
      throw Exception(stream);
    }

    return ClientPool::create(
      logger_.in(),
      scheduler_,
      channels_,
      number_client_,
      *current_task_processor);
  }

private:
  void initialize(
    const ConfigPoolCoro& config_pool,
    const std::size_t number_thread)
  {
    channels_ = UServerUtils::Grpc::Core::Client::Internal::create_channels(
      number_thread,
      config_pool.credentials,
      config_pool.endpoint,
      config_pool.channel_args,
      config_pool.number_channels);

    const auto& number_async_client = config_pool.number_async_client;
    const std::size_t adding = number_async_client % number_thread != 0;
    number_client_ =
      (adding + number_async_client / number_thread) * number_thread;
  }

private:
  Logger_var logger_;

  SchedulerPtr scheduler_;

  Channels channels_;

  std::size_t number_client_ = 0;
};

} // namespace UServerUtils::Grpc

#endif // USERVER_GRPC_COBRAZZPOOLCLIENTFACTORY_HPP
