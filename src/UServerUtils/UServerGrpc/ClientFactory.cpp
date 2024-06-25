// USERVER
#include <ugrpc/client/impl/client_configs.hpp>
#include <userver/dynamic_config/value.hpp>
#include <userver/ugrpc/client/client_qos.hpp>
#include <ugrpc/server/impl/server_configs.hpp>

// THIS
#include <UServerUtils/UServerGrpc/ClientFactory.hpp>

namespace UServerUtils::UServerGrpc
{

GrpcClientFactory::GrpcClientFactory(
  ClientFactoryConfig&& config,
  TaskProcessor& channel_task_processor,
  CompletionQueue& completion_queue,
  StatisticsStorage& statistics_storage,
  const MiddlewareFactories& middleware_factories)
  : testsuite_grpc_({}, false)
{
  userver::ugrpc::client::ClientFactoryConfig client_config;
  client_config.channel_args = std::move(config.channel_args);
  client_config.channel_count = config.channel_count;
  client_config.credentials = grpc::InsecureChannelCredentials();
  client_config.native_log_level = userver::logging::Level::kError;

  const auto& docs_map = registrator_dynamic_settings_.docs_map();
  storage_mock_ = StorageMockPtr(new StorageMock(
    docs_map,
    {
      {userver::ugrpc::client::impl::kEnforceClientTaskDeadline, config.enable_deadline_propagation}
    }));

  client_factory_ =
    std::make_unique<ClientFactory>(
      std::move(client_config),
      channel_task_processor,
      middleware_factories,
      completion_queue,
      statistics_storage,
      testsuite_grpc_,
      storage_mock_->GetSource());
}

} // namespace UServerUtils::UServerGrpc