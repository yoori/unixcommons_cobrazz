#ifndef GRPC_CORE_SERVER_CONFIG_H_
#define GRPC_CORE_SERVER_CONFIG_H_

// STD
#include <string>
#include <unordered_map>

// THIS
#include <UServerUtils/Grpc/Core/Server/CommonContext.hpp>

namespace UServerUtils::Grpc::Core::Server
{

enum class RequestHandlerType
{
  Copy,
  Move
};

struct Config final
{
  Config() = default;

  ~Config() = default;

  std::size_t port = 0;

  std::string ip = "0.0.0.0";

  // If not set, then find number of threads
  // at which performance will be maximum.
  std::optional<std::size_t> num_threads = {};

  // Optional grpc-core channel args
  // @see https://grpc.github.io/grpc/core/group__grpc__arg__keys.html
  std::unordered_map<std::string, std::string> channel_args;

  CommonContext_var common_context;

  // Determines which request handler is called
  // on_request(const Request&) or on_request(std::unique_ptr<RequestPtr>&&)
  RequestHandlerType request_handler_type = RequestHandlerType::Copy;
};

} // namespace UServerUtils::Grpc::Core::Server

#endif // GRPC_CORE_SERVER_CONFIG_H_
