// USERVER
#include <userver/ugrpc/client/client_factory_settings.hpp>

// USERVER PRIVATE HEADERS
#include <ugrpc/client/impl/client_configs.hpp>

// THIS
#include <UServerUtils/UServerGrpc/ClientFactory.hpp>

namespace UServerUtils::UServerGrpc
{

GrpcClientFactory::GrpcClientFactory(
  ClientFactoryConfig&& config,
  TaskProcessor& channel_task_processor,
  StatisticsStorage& statistics_storage,
  const CompletionQueuePoolBasePtr& queue_pool,
  const MiddlewareFactories& middleware_factories)
  : grpc_statistics_storage_(
      statistics_storage,
      userver::ugrpc::impl::StatisticsDomain::kClient),
    testsuite_grpc_({}, false)
{
  ClientFactorySettings client_settings;
  client_settings.credentials = std::move(config.credentials);
  client_settings.client_credentials = std::move(config.client_credentials);
  client_settings.channel_args = std::move(config.channel_args);
  client_settings.channel_count = config.channel_count;
  client_settings.native_log_level = userver::logging::Level::kError;

  const auto& docs_map = registrator_dynamic_settings_.docs_map();
  storage_mock_ = StorageMockPtr(new StorageMock(
    docs_map,
    {
      {
        userver::ugrpc::client::impl::kEnforceClientTaskDeadline,
        config.enable_deadline_propagation
      }
    }));

  client_factory_ = std::make_unique<ClientFactory>(
    std::move(client_settings),
    channel_task_processor,
    middleware_factories,
    *queue_pool,
    grpc_statistics_storage_,
    testsuite_grpc_,
    storage_mock_->GetSource());
}

} // namespace UServerUtils::UServerGrpc
