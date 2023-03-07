// THIS
#include "Rpc.hpp"
#include "RpcImpl.hpp"
#include "Service.hpp"

namespace UServerUtils::Grpc::Core::Server
{

namespace Aspect
{
const char SERVICE[] = "SCHEDULER";
}

Service::Service(
  const Logger_var& logger,
  const RpcPool_var& rpc_pool,
  const ServerCompletionQueues& server_completion_queues,
  const CommonContext_var& common_context,
  Handlers&& handlers)
  : logger_(logger),
    rpc_pool_(rpc_pool),
    server_completion_queues_(server_completion_queues),
    common_context_(common_context),
    handlers_(std::move(handlers))
{
  for (const auto& handler : handlers_)
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
    Stream::Error stream;
    bool error = false;

    if (state_ == AS_ACTIVE)
    {
      stream << FNS
             << ": wasn't deactivated.";
      error = true;
    }

    if (state_ != AS_NOT_ACTIVE)
    {
      if (error)
      {
        stream << std::endl;
      }
      stream << FNS
             << ": didn't wait for deactivation, still active.";
      error = true;
    }

    if (error)
    {
      logger_->error(stream.str(), Aspect::SERVICE);
    }
  }
  catch (const eh::Exception& exc)
  {
    try
    {
      std::cerr << FNS
                << ": eh::Exception: "
                << exc.what()
                << std::endl;
    }
    catch (...)
    {
    }
  }
}

void Service::activate_object()
{
  std::lock_guard lock(state_mutex_);
  if (state_ != AS_NOT_ACTIVE)
  {
    Stream::Error stream;
    stream << FNS
           << ": already active";
    throw ActiveObject::AlreadyActive(stream);
  }

  try
  {
    int method_index = 0;
    for (const auto& handler : handlers_)
    {
      for (auto& server_completion_queue : server_completion_queues_)
      {
        std::shared_ptr<Rpc> rpc =
          RpcImpl::create(
            logger_,
            server_completion_queue,
            method_index,
            handler.second,
            common_context_,
            *this,
            *rpc_pool_);
        rpc_pool_->add(rpc);
      }

      method_index += 1;
    }

    state_ = AS_ACTIVE;
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error stream;
    stream << FNS
           << ": activate_object failure: "
           << exc.what();
    throw Exception(stream);
  }
}

void Service::deactivate_object()
{
  {
    std::unique_lock lock(state_mutex_);
    if (state_ == AS_ACTIVE)
    {
      state_ = AS_DEACTIVATING;
    }
  }

  condition_variable_.notify_all();
}

void Service::wait_object()
{
  std::unique_lock lock(state_mutex_);
  condition_variable_.wait(lock, [this] () {
    return state_ != AS_ACTIVE;
  });

  if (state_ == AS_DEACTIVATING)
  {
    state_ = AS_NOT_ACTIVE;
  }
}

bool Service::active()
{
  std::lock_guard lock(state_mutex_);
  return state_ == AS_ACTIVE;
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
