#ifndef GRPC_CORE_SERVER_SERVICE_H_
#define GRPC_CORE_SERVER_SERVICE_H_

// STD
#include <string_view>
#include <condition_variable>

// GRPC
#include <grpcpp/impl/codegen/proto_utils.h>

// THIS
#include <Generics/ActiveObject.hpp>
#include <Logger/Logger.hpp>
#include <UServerUtils/Grpc/Core/Server/CommonContext.hpp>
#include <UServerUtils/Grpc/Core/Server/RpcDelegate.hpp>
#include <UServerUtils/Grpc/Core/Server/RpcHandlerInfo.hpp>
#include <UServerUtils/Grpc/Core/Server/RpcPool.hpp>

namespace UServerUtils::Grpc::Core::Server
{

class Service final :
  public RpcDelegate,
  public grpc::Service,
  public Generics::ActiveObject,
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

  void activate_object() override;

  void deactivate_object() override;

  void wait_object() override;

  bool active() override;

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
  ~Service() override;

private:
  Logger_var logger_;

  RpcPool_var rpc_pool_;

  ServerCompletionQueues server_completion_queues_;

  CommonContext_var common_context_;

  Handlers handlers_;

  ACTIVE_STATE state_ = AS_NOT_ACTIVE;

  std::mutex state_mutex_;

  std::condition_variable condition_variable_;
};

using Service_var = ReferenceCounting::SmartPtr<Service>;

} // namespace UServerUtils::Grpc::Core::Server

#endif // GRPC_CORE_SERVER_SERVICE_H_