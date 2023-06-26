// USERVER
#include <ugrpc/server/impl/server_configs.hpp>
#include <userver/dynamic_config/value.hpp>
#include <ugrpc/client/impl/client_configs.hpp>

// THIS
#include <UServerUtils/Grpc/ServerBuilder.hpp>

namespace UServerUtils::Grpc
{

GrpcServerBuilder::GrpcServerBuilder(
  Logger* logger,
  GrpcServerConfig&& config,
  StatisticsStorage& statistics_storage,
  const RegistratorDynamicSettingsPtr& registrator_dynamic_settings)
{
  GrpcServer::ServerConfig server_config;
  server_config.port = std::move(config.port);
  server_config.channel_args = std::move(config.channel_args);
  server_config.enable_channelz = config.enable_channelz;
  server_config.native_log_level = userver::logging::Level::kError;

  const auto& docs_map = registrator_dynamic_settings->docs_map();
  StorageMockPtr storage_mock(new StorageMock(
    docs_map,
    {
      {userver::ugrpc::server::impl::kServerCancelTaskByDeadline, config.cancel_task_by_deadline}
    }));

  grpc_server_ = GrpcServer_var(
    new GrpcServer(
      logger,
      std::move(server_config),
      statistics_storage,
      std::move(storage_mock)));
}

GrpcServerBuilder::CompletionQueue&
GrpcServerBuilder::get_completion_queue()
{
  if (!grpc_server_)
  {
    Stream::Error stream;
    stream << FNS
           << ": grpc_server is null";
    throw Exception(stream);
  }

  return grpc_server_->get_completion_queue();
}

void GrpcServerBuilder::add_grpc_service(
  TaskProcessor& task_processor,
  GrpcServiceBase* service,
  const Middlewares& middlewares)
{
  if (!grpc_server_)
  {
    Stream::Error stream;
    stream << FNS
           << ": grpc_server is null";
    throw Exception(stream);
  }

  middlewares_list_.emplace_back(std::make_unique<Middlewares>(middlewares));
  services_.emplace_back(
    GrpcServiceBase_var(ReferenceCounting::add_ref(service)));

  grpc_server_->add_service(
    *service,
    task_processor,
    *middlewares_list_.back());
}

GrpcServerBuilder::ServerInfo
GrpcServerBuilder::build()
{
  ServerInfo server_info;
  server_info.server = std::move(grpc_server_);
  server_info.services = std::move(services_);
  server_info.middlewares_list = std::move(middlewares_list_);

  return server_info;
}

} // namespace UServerUtils::Grpc