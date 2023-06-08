// THIS
#include <UServerUtils/Grpc/ServerBuilder.hpp>

namespace UServerUtils::Grpc
{

GrpcServerBuilder::GrpcServerBuilder(
  const Logger_var& logger,
  GrpcServerConfig&& config,
  StatisticsStorage& statistics_storage)
{
  GrpcServer::ServerConfig server_config;
  server_config.port = std::move(config.port);
  server_config.channel_args = std::move(config.channel_args);
  server_config.enable_channelz = config.enable_channelz;
  server_config.native_log_level = userver::logging::Level::kError;

  grpc_server_ = GrpcServer_var(
    new GrpcServer(
      logger,
      std::move(server_config),
      statistics_storage));
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
  GrpcServiceBase_var&& service)
{
  if (!grpc_server_)
  {
    Stream::Error stream;
    stream << FNS
           << ": grpc_server is null";
    throw Exception(stream);
  }

  grpc_server_->add_service(*service, task_processor);
  services_.emplace_back(std::move(service));
}

GrpcServerBuilder::ServerInfo
GrpcServerBuilder::build()
{
  ServerInfo server_info;
  server_info.server = std::move(grpc_server_);
  server_info.services = std::move(services_);

  return server_info;
}

} // namespace UServerUtils::Grpc