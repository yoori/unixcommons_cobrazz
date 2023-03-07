#ifndef GRPC_CORE_SERVER_CONFIG_CORO_H_
#define GRPC_CORE_SERVER_CONFIG_CORO_H_

// STD
#include <string>
#include <unordered_map>

namespace UServerUtils::Grpc::Core::Server
{

struct ConfigCoro final
{
  ConfigCoro() = default;

  ~ConfigCoro() = default;

  std::size_t port = 0;

  std::string ip = "0.0.0.0";

  // If not set, then find number of threads
  // at which performance will be maximum.
  std::optional<std::size_t> num_threads = {};

  // Optional grpc-core channel args
  // @see https://grpc.github.io/grpc/core/group__grpc__arg__keys.html
  std::unordered_map<std::string, std::string> channel_args;

  std::optional<std::size_t> max_size_queue = {};
};

} // namespace UServerUtils::Grpc::Core::Server

#endif // GRPC_CORE_SERVER_CONFIG_CORO_H_