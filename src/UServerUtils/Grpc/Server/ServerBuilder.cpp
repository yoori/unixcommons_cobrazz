// THIS
#include <UServerUtils/Grpc/Server/ServerBuilder.hpp>

namespace UServerUtils::Grpc::Server
{

ServerBuilder::ServerBuilder(
  const Config& config,
  Logger* logger)
  : grpc_server_(new Grpc::Server::ServerCoro(config, logger))
{
}

ServerBuilder::ServerInfo ServerBuilder::build()
{
  ServerInfo server_info;
  server_info.server = std::move(grpc_server_);
  server_info.services = std::move(services_);

  return server_info;
}

} // namespace UServerUtils::Grpc::Server