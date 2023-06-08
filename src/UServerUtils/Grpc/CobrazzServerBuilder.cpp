// THIS
#include <UServerUtils/Grpc/CobrazzServerBuilder.hpp>

namespace UServerUtils::Grpc
{

GrpcCobrazzServerBuilder::GrpcCobrazzServerBuilder(
  const Config& config,
  const Logger_var& logger)
  : grpc_server_(new Core::Server::ServerCoro(config, logger))
{
}

GrpcCobrazzServerBuilder::ServerInfo
GrpcCobrazzServerBuilder::build()
{
  ServerInfo server_info;
  server_info.server = std::move(grpc_server_);
  server_info.services = std::move(services_);

  return server_info;
}

} // namespace UServerUtils::Grpc