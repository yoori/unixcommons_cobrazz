#ifndef GRPC_SERVER_RPC_HANDLER_INFO_H_
#define GRPC_SERVER_RPC_HANDLER_INFO_H_

// STD
#include <functional>
#include <string>

// PROTOBUF
#include <google/protobuf/message.h>

// GRPC
#include <grpcpp/impl/rpc_method.h>

// THIS
#include <UServerUtils/Grpc/Server/CommonContext.hpp>
#include <UServerUtils/Grpc/Server/Config.hpp>
#include <UServerUtils/Grpc/Server/Rpc.hpp>
#include <UServerUtils/Grpc/Server/RpcHandler.hpp>

namespace UServerUtils::Grpc::Server
{

using RpcHandlerFactory =
  std::function<std::unique_ptr<RpcHandler>(
    Rpc*,
    CommonContext*)>;

struct RpcHandlerInfo final
{
  RpcHandlerInfo(
    const google::protobuf::Descriptor* request_descriptor,
    const google::protobuf::Descriptor* response_descriptor,
    const grpc::internal::RpcMethod::RpcType rpc_type,
    const RequestHandlerType request_handler_type,
    RpcHandlerFactory&& rpc_handler_factory,
    const std::string_view method_full_name)
    : request_descriptor(request_descriptor),
      response_descriptor(response_descriptor),
      rpc_type(rpc_type),
      request_handler_type(request_handler_type),
      rpc_handler_factory(std::move(rpc_handler_factory)),
      method_full_name(method_full_name) {
  }

  ~RpcHandlerInfo() = default;

  const google::protobuf::Descriptor* request_descriptor;
  const google::protobuf::Descriptor* response_descriptor;
  const grpc::internal::RpcMethod::RpcType rpc_type;
  const RequestHandlerType request_handler_type;
  const RpcHandlerFactory rpc_handler_factory;
  const std::string_view method_full_name;
};

} // UServerUtils::Grpc::Server

#endif //GRPC_SERVER_RPC_HANDLER_INFO_H_