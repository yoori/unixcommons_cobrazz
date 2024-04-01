#ifndef GRPC_CORE_SERVER_RPCIMPL_H_
#define GRPC_CORE_SERVER_RPCIMPL_H_

// GRPC
#include <grpcpp/notifier.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/impl/codegen/async_unary_call.h>
#include <grpcpp/impl/codegen/async_stream.h>
#include <grpcpp/impl/codegen/proto_utils.h>

// PROTOBUF
#include <google/protobuf/message.h>

// STD
#include <atomic>
#include <memory>
#include <thread>

// THIS
#include <eh/Exception.hpp>
#include <Logger/Logger.hpp>
#include <UServerUtils/Grpc/Core/Server/CommonContext.hpp>
#include <UServerUtils/Grpc/Core/Server/Event.hpp>
#include <UServerUtils/Grpc/Core/Server/EventObserver.hpp>
#include <UServerUtils/Grpc/Core/Server/PendingQueue.hpp>
#include <UServerUtils/Grpc/Core/Server/Rpc.hpp>
#include <UServerUtils/Grpc/Core/Server/RpcDelegate.hpp>
#include <UServerUtils/Grpc/Core/Server/RpcHandlerInfo.hpp>
#include <UServerUtils/Grpc/Core/Server/RpcPool.hpp>

namespace UServerUtils::Grpc::Core::Server
{

class RpcImpl final :
  public Rpc,
  public EventObserver,
  public EventQueueObserver,
  public std::enable_shared_from_this<RpcImpl>
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using ServerCompletionQueuePtr =
    std::shared_ptr<grpc::ServerCompletionQueue>;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

private:
  using Message = google::protobuf::Message;
  using MessagePtr = std::unique_ptr<Message>;

  using ServerAsyncResponseWriter = grpc::ServerAsyncResponseWriter<Message>;
  using ServerAsyncResponseWriterPtr = std::unique_ptr<ServerAsyncResponseWriter>;

  using ServerAsyncReader = grpc::ServerAsyncReader<Message, Message>;
  using ServerAsyncReaderPtr = std::unique_ptr<ServerAsyncReader>;

  using ServerAsyncReaderWriter = grpc::ServerAsyncReaderWriter<Message, Message>;
  using ServerAsyncReaderWriterPtr = std::unique_ptr<ServerAsyncReaderWriter>;

  using ServerAsyncWriter = grpc::ServerAsyncWriter<Message>;
  using ServerAsyncWriterPtr = std::unique_ptr<ServerAsyncWriter>;

  using RpcHandlerPtr = std::unique_ptr<RpcHandler>;
  using StatusOptional = std::optional<grpc::Status>;

  enum class RpcState
  {
    Idle,
    Connected,
    Closed,
    Stopped
  };

public:
  static std::shared_ptr<Rpc> create(
    Logger* logger,
    const ServerCompletionQueuePtr& server_completion_queue,
    const int method_index,
    const RpcHandlerInfo& rpc_handler_info,
    CommonContext* common_context,
    RpcDelegate& delegate,
    RpcPool& rpc_pool);

  ~RpcImpl() override = default;

  bool write(MessagePtr&& message) noexcept override;

  bool finish(grpc::Status&& status) noexcept override;

  bool stop() noexcept override;

  bool is_stopped() noexcept override;

  std::weak_ptr<Rpc> get_weak_ptr() noexcept override;

private:
  explicit RpcImpl(
    Logger* logger,
    const ServerCompletionQueuePtr& server_completion_queue,
    const int method_index,
    const RpcHandlerInfo& rpc_handler_info,
    CommonContext* common_context,
    RpcDelegate& delegate,
    RpcPool& rpc_pool);

  void initialize();

  void on_event(
    const bool ok,
    const EventType type) noexcept override;

  void on_event_queue(
    const bool ok,
    PendingQueueData&& data) noexcept override;

  void on_connection(const bool ok) noexcept;

  void on_notify(const bool ok) noexcept;

  void on_read(const bool ok) noexcept;

  void on_write(const bool ok) noexcept;

  void on_finish(const bool ok) noexcept;

  void on_done(const bool ok) noexcept;

  void read_if_needed() noexcept;

  void try_close() noexcept;

  void execute_queue() noexcept;

  void execute_queue_stream(
    const PendingQueueType type,
    const StatusOptional& status,
    MessagePtr&& message);

  void execute_unique(
    const StatusOptional& status,
    MessagePtr&& message);

private:
  Logger_var logger_;

  ServerCompletionQueuePtr server_completion_queue_;

  const int method_index_;

  const RpcHandlerInfo rpc_handler_info_;

  CommonContext_var common_context_;

  RpcDelegate& delegate_;

  RpcPool& rpc_pool_;

  PendingQueue pending_queue_;

  Event connection_event_;

  Event read_event_;

  Event write_event_;

  Event finish_event_;

  Event done_event_;

  std::thread::id thread_id_ = std::thread::id();

  grpc::Notifier notifier_;

  const Message* request_message_prototype_;

  MessagePtr request_;

  MessagePtr response_;

  grpc::ServerContext server_context_;

  ServerAsyncResponseWriterPtr server_async_response_writer_;

  ServerAsyncReaderPtr server_async_reader_;

  ServerAsyncReaderWriterPtr server_async_reader_writer_;

  ServerAsyncWriterPtr server_async_writer_;

  RpcHandlerPtr handler_;

  RpcState rpc_state_ = RpcState::Idle;

  bool is_finished_ = false;

  std::atomic<bool> is_stopped_{false};
};

} // namespace UServerUtils::Grpc::Core::Server

#include "RpcImpl.ipp"

#endif // GRPC_CORE_SERVER_RPCIMPL_H_
