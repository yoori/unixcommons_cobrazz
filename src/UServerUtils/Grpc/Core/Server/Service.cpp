// THIS
#include <UServerUtils/Grpc/Core/Server/Rpc.hpp>
#include <UServerUtils/Grpc/Core/Server/RpcImpl.hpp>
#include <UServerUtils/Grpc/Core/Server/Service.hpp>

namespace UServerUtils::Grpc::Core::Server
{
  namespace Aspect
  {
    const char SERVICE[] = "SERVICE";
  }

  Service::Service(
    Logger* logger,
    RpcPool* rpc_pool,
    const ServerCompletionQueues& server_completion_queues,
    CommonContext* common_context,
    Handlers&& handlers)
    : logger_(ReferenceCounting::add_ref(logger)),
      rpc_pool_(ReferenceCounting::add_ref(rpc_pool)),
      server_completion_queues_(server_completion_queues),
      common_context_(ReferenceCounting::add_ref(common_context)),
      handlers_(std::move(handlers))
  {
    for (const auto& handler: handlers_)
    {
      grpc::Service::AddMethod(
        new grpc::internal::RpcServiceMethod(
          handler.second.method_full_name.data(),
          handler.second.rpc_type,
          nullptr));
    }
  }

  Service::~Service()
  {
    try
    {
      if (active())
      {
        deactivate_object();
        wait_object();
      }
    }
    catch (const eh::Exception& exc)
    {
      std::cerr << FNS << ": eh::Exception: " << exc.what() << std::endl;
    }
  }

  void Service::activate_object_()
  {
    try
    {
      std::lock_guard lock(mutex_);

      int method_index = 0;
      for (const auto& handler: handlers_)
      {
        for (auto& server_completion_queue : server_completion_queues_)
        {
          std::shared_ptr<Rpc> rpc = RpcImpl::create(
            logger_.in(),
            server_completion_queue,
            method_index,
            handler.second,
            common_context_.in(),
            *this,
            *rpc_pool_.in());
          rpc_pool_->add(rpc);
        }

        method_index += 1;
      }
    }
    catch (const eh::Exception& exc)
    {
      Stream::Error stream;
      stream << FNS << ": activate_object failure: " << exc.what();
      throw Exception(stream);
    }
  }

  void Service::request_async_bidi_streaming(
    int index,
    grpc::ServerContext* context,
    grpc::internal::ServerAsyncStreamingInterface* stream,
    grpc::CompletionQueue* call_cq,
    grpc::ServerCompletionQueue* notification_cq,
    void* tag)
  {
    grpc::Service::RequestAsyncBidiStreaming(
      index,
      context,
      stream,
      call_cq,
      notification_cq,
      tag);
  }

  void Service::request_async_client_streaming(
    int index,
    grpc::ServerContext* context,
    grpc::internal::ServerAsyncStreamingInterface* stream,
    grpc::CompletionQueue* call_cq,
    grpc::ServerCompletionQueue* notification_cq,
    void* tag)
  {
    grpc::Service::RequestAsyncClientStreaming(
      index,
      context,
      stream,
      call_cq,
      notification_cq,
      tag);
  }

  void Service::request_async_unary(
    int index,
    grpc::ServerContext* context,
    google::protobuf::Message* request,
    grpc::internal::ServerAsyncStreamingInterface* stream,
    grpc::CompletionQueue* call_cq,
    grpc::ServerCompletionQueue* notification_cq,
    void* tag)
  {
    grpc::Service::RequestAsyncUnary(
      index,
      context,
      request,
      stream,
      call_cq,
      notification_cq,
      tag);
  }

  void Service::request_async_server_streaming(
    int index,
    grpc::ServerContext* context,
    google::protobuf::Message* request,
    grpc::internal::ServerAsyncStreamingInterface* stream,
    grpc::CompletionQueue* call_cq,
    grpc::ServerCompletionQueue* notification_cq,
    void* tag)
  {
    grpc::Service::RequestAsyncServerStreaming(
      index,
      context,
      request,
      stream,
      call_cq,
      notification_cq,
      tag);
  }
} // namespace UServerUtils::Grpc::Core::Server
