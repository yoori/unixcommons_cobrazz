#ifndef GRPC_SERVER_SERVICE_H_
#define GRPC_SERVER_SERVICE_H_

// STD
#include <string_view>
#include <condition_variable>

// GRPC
#include <grpcpp/impl/codegen/proto_utils.h>

// THIS
#include <Generics/ActiveObject.hpp>
#include <Logger/Logger.hpp>
#include <UServerUtils/Grpc/Server/CommonContext.hpp>
#include <UServerUtils/Grpc/Server/RpcDelegate.hpp>
#include <UServerUtils/Grpc/Server/RpcHandlerInfo.hpp>
#include <UServerUtils/Grpc/Server/RpcPool.hpp>

namespace UServerUtils::Grpc::Server
{

class Service final :
  public RpcDelegate,
  public grpc::Service,
  public Generics::SimpleActiveObject,
  public ReferenceCounting::AtomicImpl
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using MethodName = std::string_view;
  using Handler = std::pair<MethodName, RpcHandlerInfo>;
  using Handlers = std::list<Handler>;
  using ServerCompletionQueuePtr =
    std::shared_ptr<grpc::ServerCompletionQueue>;
  using ServerCompletionQueues =
    std::vector<ServerCompletionQueuePtr>;

public:
  Service(
    Logger* logger,
    RpcPool* rpc_pool,
    const ServerCompletionQueues& server_completion_queues,
    CommonContext* common_context,
    Handlers&& handlers);

  void request_async_bidi_streaming(
    int index,
    grpc::ServerContext* context,
    grpc::internal::ServerAsyncStreamingInterface* stream,
    grpc::CompletionQueue* call_cq,
    grpc::ServerCompletionQueue* notification_cq,
    void* tag) override;

  void request_async_client_streaming(
    int index,
    grpc::ServerContext* context,
    grpc::internal::ServerAsyncStreamingInterface* stream,
    grpc::CompletionQueue* call_cq,
    grpc::ServerCompletionQueue* notification_cq,
    void* tag) override;

  void request_async_unary(
    int index,
    grpc::ServerContext* context,
    google::protobuf::Message* request,
    grpc::internal::ServerAsyncStreamingInterface* stream,
    grpc::CompletionQueue* call_cq,
    grpc::ServerCompletionQueue* notification_cq,
    void* tag) override;

  void request_async_server_streaming(
    int index,
    grpc::ServerContext* context,
    google::protobuf::Message* request,
    grpc::internal::ServerAsyncStreamingInterface* stream,
    grpc::CompletionQueue* call_cq,
    grpc::ServerCompletionQueue* notification_cq,
    void* tag) override;

protected:
  ~Service() override = default;

  void activate_object_() override;

private:
  Logger_var logger_;

  RpcPool_var rpc_pool_;

  ServerCompletionQueues server_completion_queues_;

  CommonContext_var common_context_;

  Handlers handlers_;
};

using Service_var = ReferenceCounting::SmartPtr<Service>;

} // namespace UServerUtils::Grpc::Server

#endif // GRPC_SERVER_SERVICE_H_