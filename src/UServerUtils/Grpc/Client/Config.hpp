#ifndef GRPC_CLIENT_CLIENT_CONFIG_H_
#define GRPC_CLIENT_CLIENT_CONFIG_H_

// STD
#include <memory>
#include <optional>

// GRPCPP
#include <grpcpp/security/credentials.h>

namespace UServerUtils::Grpc::Client
{

struct Config final
{
  Config() = default;

  ~Config() = default;

  std::string endpoint;

  std::shared_ptr<grpc::ChannelCredentials> credentials =
    grpc::InsecureChannelCredentials();

  // Optional grpc-core channel args
  // @see https://grpc.github.io/grpc/core/group__grpc__arg__keys.html
  std::unordered_map<std::string, std::string> channel_args;

  // If not set, then find number of threads
  // at which performance will be maximum.
  std::optional<std::size_t> number_threads = {};

  // Desired number of channels. Result number of channels
  // is rounded up to a multiple of the number of threads.
  // If number of channels is not set, then
  // it is equal to the number of threads.
  std::optional<std::size_t> number_channels = {};
};

} // namespace UServerUtils::Grpc::Client

#endif // GRPC_CLIENT_CLIENT_CONFIG_H_
