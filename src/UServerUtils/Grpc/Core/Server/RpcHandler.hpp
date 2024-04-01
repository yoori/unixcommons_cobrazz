#ifndef GRPC_CORE_SERVER_RPC_HANDLER_H_
#define GRPC_CORE_SERVER_RPC_HANDLER_H_

// PROTOBUF
#include <google/protobuf/message.h>

// THIS
#include <Generics/Uncopyable.hpp>
#include <UServerUtils/Grpc/Core/Server/CommonContext.hpp>
#include <UServerUtils/Grpc/Core/Server/Rpc.hpp>

namespace UServerUtils::Grpc::Core::Server
{

class RpcHandler : protected Generics::Uncopyable
{
public:
  using Message = google::protobuf::Message;
  using MessagePtr = std::unique_ptr<Message>;

public:
  virtual ~RpcHandler() = default;

  virtual void initialize() = 0;

  virtual void on_reads_done() = 0;

  virtual void on_finish() = 0;

  virtual void set_rpc(Rpc* rpc) noexcept = 0;

  virtual void set_common_context(
    CommonContext* common_context) noexcept = 0;

protected:
  virtual void on_request_internal(const Message& request) = 0;

  virtual void on_request_internal(MessagePtr&& request) = 0;

  RpcHandler() = default;

  friend class RpcImpl;
};

} // namespace UServerUtils::Grpc::Core::Server

#endif // GRPC_CORE_SERVER_RPC_HANDLER_H_
