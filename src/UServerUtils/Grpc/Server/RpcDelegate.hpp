#ifndef GRPC_SERVER_RPC_DELEGATE_H_
#define GRPC_SERVER_RPC_DELEGATE_H_

// GRPC
#include <grpcpp/grpcpp.h>
#include <grpcpp/support/async_stream.h>

// PROTO
#include <google/protobuf/message.h>

// THIS
#include <Generics/Uncopyable.hpp>

namespace UServerUtils::Grpc::Server
{

class RpcDelegate : protected Generics::Uncopyable
{
public:
  virtual void request_async_bidi_streaming(
    int index,
    grpc::ServerContext* context,
    grpc::internal::ServerAsyncStreamingInterface* stream,
    grpc::CompletionQueue* call_cq,
    grpc::ServerCompletionQueue* notification_cq,
    void* tag) = 0;

  virtual void request_async_client_streaming(
    int index,
    grpc::ServerContext* context,
    grpc::internal::ServerAsyncStreamingInterface* stream,
    grpc::CompletionQueue* call_cq,
    grpc::ServerCompletionQueue* notification_cq,
    void* tag) = 0;

  virtual void request_async_unary(
    int index,
    grpc::ServerContext* context,
    google::protobuf::Message* request,
    grpc::internal::ServerAsyncStreamingInterface* stream,
    grpc::CompletionQueue* call_cq,
    grpc::ServerCompletionQueue* notification_cq,
    void* tag) = 0;

  virtual void request_async_server_streaming(
    int index,
    grpc::ServerContext* context,
    google::protobuf::Message* request,
    grpc::internal::ServerAsyncStreamingInterface* stream,
    grpc::CompletionQueue* call_cq,
    grpc::ServerCompletionQueue* notification_cq,
    void* tag) = 0;

protected:
  RpcDelegate() = default;

  virtual ~RpcDelegate() = default;
};

} // namespace UServerUtils::Grpc::Server

#endif //GRPC_SERVER_RPC_DELEGATE_H_
