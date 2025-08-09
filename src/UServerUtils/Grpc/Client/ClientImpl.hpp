#ifndef GRPC_CLIENT_CLIENT_IMPL_H_
#define GRPC_CLIENT_CLIENT_IMPL_H_

// STD
#include <memory>

// GRPC
#include <grpcpp/grpcpp.h>
#include <grpcpp/notifier.h>
#include <grpcpp/impl/codegen/async_stream.h>
#include <grpcpp/impl/codegen/async_unary_call.h>

// THIS
#include <Generics/Uncopyable.hpp>
#include <Logger/Logger.hpp>
#include <UServerUtils/Grpc/Client/Client.hpp>
#include <UServerUtils/Grpc/Client/ClientDelegate.hpp>
#include <UServerUtils/Grpc/Client/ClientObserver.hpp>
#include <UServerUtils/Grpc/Client/Event.hpp>
#include <UServerUtils/Grpc/Client/EventObserver.hpp>
#include <UServerUtils/Grpc/Client/EventType.hpp>
#include <UServerUtils/Grpc/Client/PendingQueue.hpp>
#include <UServerUtils/Grpc/Client/Types.hpp>
#include <UServerUtils/Grpc/Common/RpcServiceMethodTraits.hpp>

namespace UServerUtils::Grpc::Client
{

template<class RpcServiceMethodConcept>
class ClientImpl final:
  public Client<Internal::Request<RpcServiceMethodConcept>>,
  public EventObserver,
  public EventQueueObserver,
  public std::enable_shared_from_this<ClientImpl<RpcServiceMethodConcept>>
{
public:
  using Traits = Internal::Common::RpcServiceMethodTraits<
    RpcServiceMethodConcept>;
  using Request = typename Traits::Request;
  using RequestPtr = std::unique_ptr<Request>;
  using Response = typename Traits::Response;
  using ResponsePtr = std::unique_ptr<Response>;
  using Delegate = ClientDelegate<Request>;
  using Observer = ClientObserver<RpcServiceMethodConcept>;
  using ObserverPtr = std::shared_ptr<Observer>;
  using ChannelPtr = std::shared_ptr<grpc::Channel>;
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;

  using CompletionQueuePtr = std::shared_ptr<grpc::CompletionQueue>;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

private:
  using Message = google::protobuf::Message;
  using MessagePtr = std::unique_ptr<Message>;
  using ClientPtr = std::shared_ptr<Client<Request>>;

  using ClientAsyncReaderWriter =
    grpc::ClientAsyncReaderWriter<
      google::protobuf::Message,
      google::protobuf::Message>;
  using ClientAsyncReaderWriterPtr =
    std::unique_ptr<ClientAsyncReaderWriter>;

  using ClientAsyncReader =
    grpc::ClientAsyncReader<google::protobuf::Message>;
  using ClientAsyncReaderPtr =
    std::unique_ptr<ClientAsyncReader>;

  using ClientAsyncWriter =
    grpc::ClientAsyncWriter<google::protobuf::Message>;
  using ClientAsyncWriterPtr =
    std::unique_ptr<ClientAsyncWriter>;

  using ClientAsyncResponseReader =
    grpc::ClientAsyncResponseReader<google::protobuf::Message>;
  using ClientAsyncResponseReaderPtr =
    std::unique_ptr<ClientAsyncResponseReader>;

private:
  static constexpr auto k_rpc_type = Traits::rpc_type;

  enum class RpcState
  {
    Idle,
    Initialize,
    WritesDone,
    Finish,
    Error,
    Stop
  };

public:
  static ClientPtr create(
    Logger* logger,
    const ChannelPtr& channel,
    const CompletionQueuePtr& completion_queue,
    const ObserverPtr& observer,
    Delegate& delegate,
    RequestPtr&& request);

  ~ClientImpl() override = default;

  void start() noexcept;

  bool write(RequestPtr&& request) noexcept override;

  bool writes_done() noexcept override;

  ClientId get_id() const noexcept override;

  bool stop() noexcept override;

private:
  explicit ClientImpl(
    Logger* logger,
    const ChannelPtr& channel,
    const CompletionQueuePtr& completion_queue,
    const ObserverPtr& observer,
    Delegate& delegate,
    RequestPtr&& request);

  static ClientId create_id() noexcept;

  void on_start(const bool ok) noexcept;

  void on_initialize(const bool ok) noexcept;

  void on_read(const bool ok) noexcept;

  void on_write(const bool ok) noexcept;

  void on_notify(const bool ok) noexcept;

  void on_finish(const bool ok) noexcept;

  void on_stop(const bool ok) noexcept;

  void on_error(
    const std::string_view info) noexcept;

  void on_event(
    const bool ok,
    const EventType type) noexcept override;

  void on_event_queue(
    const bool ok,
    PendingQueueData&& type) noexcept override;

  void request_new_read() noexcept;

  void execute_queue() noexcept;

  void try_close() noexcept;

private:
  const ClientId client_id_;

  Logger_var logger_;

  ChannelPtr channel_;

  CompletionQueuePtr completion_queue_;

  ObserverPtr observer_;

  Delegate& delegate_;

  MessagePtr request_;

  MessagePtr responce_;

  grpc::ClientContext client_context_;

  grpc::internal::RpcMethod rpc_method_;

  PendingQueue pending_queue_;

  Event initialize_event_;

  Event read_event_;

  Event write_event_;

  Event finish_event_;

  ClientAsyncReaderWriterPtr client_reader_writer_;

  ClientAsyncReaderPtr client_reader_;

  ClientAsyncWriterPtr client_writer_;

  ClientAsyncResponseReaderPtr client_response_reader_;

  RpcState rpc_state_ = RpcState::Idle;

  grpc::Notifier notifier_;

  grpc::Status status_;
};

} // namespace UServerUtils::Grpc::Client

#include "ClientImpl.ipp"

#endif // GRPC_CLIENT_CLIENT_IMPL_H_