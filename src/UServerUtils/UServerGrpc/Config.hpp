#ifndef USERVER_USERVERGRPC_CONFIG_HPP
#define USERVER_USERVERGRPC_CONFIG_HPP

// GRPCPP
#include <grpcpp/security/credentials.h>

namespace UServerUtils::UServerGrpc
{

struct ServerConfig final
{
  // The port to listen to. If `0`, a free port will be picked automatically.
  // If none, the ports have to be configured programmatically using
  // Server::WithServerBuilder.
  std::optional<int> port{};

  // Optional grpc-core channel args
  // @see https://grpc.github.io/grpc/core/group__grpc__arg__keys.html
  std::unordered_map<std::string, std::string> channel_args;

  // Serve a web page with runtime info about gRPC connections
  bool enable_channelz = false;

  bool cancel_task_by_deadline = true;
};

struct ClientFactoryConfig final {
  // gRPC channel credentials, none by default
  std::shared_ptr<grpc::ChannelCredentials> credentials{
    grpc::InsecureChannelCredentials()};

  // Optional grpc-core channel args
  // @see https://grpc.github.io/grpc/core/group__grpc__arg__keys.html
  grpc::ChannelArguments channel_args;

  // Number of underlying channels that will be created for every client
  // in this factory.
  std::size_t channel_count = 1;

  bool enable_deadline_propagation = true;
};

} // namespace UServerUtils::UServerGrpc

#endif //USERVER_USERVERGRPC_CONFIG_HPP
