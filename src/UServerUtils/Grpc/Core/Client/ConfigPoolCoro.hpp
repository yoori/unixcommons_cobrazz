#ifndef GRPC_CORE_CLIENT_CONFIG_POOL_CORO_H_
#define GRPC_CORE_CLIENT_CONFIG_POOL_CORO_H_

// STD
#include <memory>
#include <optional>
#include <unordered_map>

// GRPCPP
#include <grpcpp/security/credentials.h>

namespace UServerUtils::Grpc::Core::Client
{

struct ConfigPoolCoro final
{
  ConfigPoolCoro() = default;

  ~ConfigPoolCoro() = default;

  std::string endpoint;

  std::shared_ptr<grpc::ChannelCredentials> credentials{
    grpc::InsecureChannelCredentials()};

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

  // Desired number of async clients. Result number of async clients
  // is rounded up to a multiple of the number of threads.
  std::size_t number_async_client = 100;
};

} // namespace UServerUtils::Grpc::Core::Client

#endif // GRPC_CORE_CLIENT_CONFIG_POOL_CORO_H_