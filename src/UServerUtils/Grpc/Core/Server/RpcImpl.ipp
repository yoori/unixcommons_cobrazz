// THIS
#include "Event.hpp"
#include "EventType.hpp"
#include "RpcImpl.hpp"

namespace UServerUtils::Grpc::Core::Server
{

namespace Aspect
{

const char RPCIMPL[] = "RPCIMPL";

} // namespace Aspect

inline std::shared_ptr<Rpc> RpcImpl::create(
  const Logger_var& logger,
  const ServerCompletionQueuePtr& server_completion_queue,
  const int method_index,
  const RpcHandlerInfo& rpc_handler_info,
  const CommonContext_var& common_context,
  RpcDelegate& delegate,
  RpcPool& rpc_pool)
{
  auto rpc = std::shared_ptr<RpcImpl>(
    new RpcImpl(
      logger,
      server_completion_queue,
      method_index,
      rpc_handler_info,
      common_context,
      delegate,
      rpc_pool));
  rpc->initialize();

  return rpc;
}

inline RpcImpl::RpcImpl(
  const Logger_var& logger,
  const ServerCompletionQueuePtr& server_completion_queue,
  const int method_index,
  const RpcHandlerInfo& rpc_handler_info,
  const CommonContext_var& common_context,
  RpcDelegate& delegate,
  RpcPool& rpc_pool)
  : logger_(logger),
    server_completion_queue_(server_completion_queue),
    method_index_(method_index),
    rpc_handler_info_(rpc_handler_info),
    common_context_(common_context),
    delegate_(delegate),
    rpc_pool_(rpc_pool),
    connection_event_(EventType::Connection, *this, false),
    read_event_(EventType::Read, *this, false),
    write_event_(EventType::Write, *this, false),
    finish_event_(EventType::Finish, *this, false),
    done_event_(EventType::Done, *this, false)
{
}

inline void RpcImpl::initialize()
{
  switch (rpc_handler_info_.rpc_type)
  {
    case grpc::internal::RpcMethod::BIDI_STREAMING:
      server_async_reader_writer_ =
        std::make_unique<ServerAsyncReaderWriter>(&server_context_);
      break;
    case grpc::internal::RpcMethod::CLIENT_STREAMING:
      server_async_reader_ =
        std::make_unique<ServerAsyncReader>(&server_context_);
      break;
    case grpc::internal::RpcMethod::NORMAL_RPC:
      server_async_response_writer_ =
        std::make_unique<ServerAsyncResponseWriter>(&server_context_);
      break;
    case grpc::internal::RpcMethod::SERVER_STREAMING:
      server_async_writer_ =
        std::make_unique<ServerAsyncWriter>(&server_context_);
      break;
  }

  // MessageFactory thread safe
  auto* message_factory = google::protobuf::MessageFactory::generated_factory();
  if (!message_factory)
  {
    Stream::Error stream;
    stream << FNS
           << ": messageFactory is null";
    throw Exception(stream);
  }

  const auto* default_request_message_prototype =
    message_factory->GetPrototype(rpc_handler_info_.request_descriptor);
  if (!default_request_message_prototype)
  {
    Stream::Error stream;
    stream << FNS
           << ": request_descriptor not supported for request of method = "
           << rpc_handler_info_.method_full_name;
    throw Exception(stream);
  }

  request_.reset(default_request_message_prototype->New());
  if (!request_)
  {
    Stream::Error stream;
    stream << FNS
           << ": request is null";
    throw Exception(stream);
  }

  server_context_.AsyncNotifyWhenDone(&done_event_);
  done_event_.set_pending(true);

  switch (rpc_handler_info_.rpc_type)
  {
    case grpc::internal::RpcMethod::BIDI_STREAMING:
      delegate_.request_async_bidi_streaming(
        method_index_,
        &server_context_,
        server_async_reader_writer_.get(),
        server_completion_queue_.get(),
        server_completion_queue_.get(),
        &connection_event_);
      break;
    case grpc::internal::RpcMethod::CLIENT_STREAMING:
      delegate_.request_async_client_streaming(
        method_index_,
        &server_context_,
        server_async_reader_.get(),
        server_completion_queue_.get(),
        server_completion_queue_.get(),
        &connection_event_);
      break;
    case grpc::internal::RpcMethod::NORMAL_RPC:
      delegate_.request_async_unary(
        method_index_,
        &server_context_,
        request_.get(),
        server_async_response_writer_.get(),
        server_completion_queue_.get(),
        server_completion_queue_.get(),
        &connection_event_);
      break;
    case grpc::internal::RpcMethod::SERVER_STREAMING:
      delegate_.request_async_server_streaming(
        method_index_,
        &server_context_,
        request_.get(),
        server_async_writer_.get(),
        server_completion_queue_.get(),
        server_completion_queue_.get(),
        &connection_event_);
      break;
  }
  connection_event_.set_pending(true);
}

inline void RpcImpl::on_event(
  const bool ok,
  const EventType type) noexcept
{
  switch (type)
  {
    case EventType::Connection:
    {
      on_connection(ok);
      break;
    }
    case EventType::Read:
    {
      on_read(ok);
      break;
    }
    case EventType::Write:
    {
      on_write(ok);
      break;
    }
    case EventType::Finish:
    {
      on_finish(ok);
      break;
    }
    case EventType::Done:
    {
      on_done(ok);
      break;
    }
  }

  try_close();
}

inline void RpcImpl::on_event_queue(
  const bool ok,
  PendingQueueData&& data) noexcept
{
  if (rpc_state_ == RpcState::Stopped)
    return;

  try
  {
    pending_queue_.emplace(std::move(data));
  }
  catch (...)
  {
  }

  execute_queue();
}

inline void RpcImpl::on_connection(const bool ok) noexcept
{
  try
  {
    connection_event_.set_pending(false);

    if (rpc_state_ == RpcState::Stopped)
      return;

    // Server-side RPC request: ok indicates that the RPC has indeed
    // been started. If it is false, the server has been Shutdown
    // before this particular call got matched to an incoming RPC.
    if (!ok)
    {
      rpc_state_ = RpcState::Closed;
      try_close();
      return;
    }

    rpc_state_ = RpcState::Connected;
    thread_id_ = std::this_thread::get_id();

    if (handler_)
    {
      try
      {
        Stream::Error stream;
        stream << FNS
               << ": Logic error. Handler must be null";
        logger_->emergency(stream.str(), Aspect::RPCIMPL);
      }
      catch (...)
      {
      }
    }

    handler_ = rpc_handler_info_.rpc_handler_factory(
      this,
      common_context_);

    try
    {
      handler_->initialize();
    }
    catch (const eh::Exception& exc)
    {
      try
      {
        Stream::Error stream;
        stream << FNS
               << ": initialize() failed, reason: "
               << exc.what();
        logger_->error(stream.str(), Aspect::RPCIMPL);
      }
      catch (...)
      {
      }
    }

    try
    {
      std::shared_ptr<Rpc> rpc = RpcImpl::create(
        logger_,
        server_completion_queue_,
        method_index_,
        rpc_handler_info_,
        common_context_,
        delegate_,
        rpc_pool_);

      rpc_pool_.add(rpc);
      read_if_needed();
    }
    catch (const eh::Exception& exc)
    {
      Stream::Error stream;
      stream << FNS
             << ": creation new rpc is failed. Reason: "
             << exc.what();
      throw Exception(stream);
    }
  }
  catch (const eh::Exception& exc)
  {
    try
    {
      Stream::Error stream;
      stream << FNS
             << ": on_connection is failed. Reason: "
             << exc.what();
      logger_->emergency(stream.str(), Aspect::RPCIMPL);
    }
    catch (...)
    {
    }
  }
}

inline void RpcImpl::on_read(const bool ok) noexcept
{
  read_event_.set_pending(false);

  if (rpc_state_ == RpcState::Stopped)
    return;

  // ok indicates whether there is a valid message
  // that got read. If not, you know that there are certainly no more
  // messages that can ever be read from this stream.
  if (ok)
  {
    try
    {
      handler_->on_request_internal(*request_);
    }
    catch (const eh::Exception& exc)
    {
      try
      {
        Stream::Error stream;
        stream << FNS
               << ": on_request is failed. Reason: "
               << exc.what();
        logger_->error(stream.str(), Aspect::RPCIMPL);
      }
      catch (...)
      {
      }
    }

    read_if_needed();
    return;
  }
  else
  {
    try
    {
      handler_->on_reads_done();
    }
    catch (const eh::Exception& exc)
    {
      try
      {
        Stream::Error stream;
        stream << FNS
               << ": on_reads_done is failed. Reason: "
               << exc.what();
        logger_->error(stream.str(), Aspect::RPCIMPL);
      }
      catch (...)
      {
      }
    }
  }
}

inline void RpcImpl::on_write(const bool ok) noexcept
{
  write_event_.set_pending(false);

  if (rpc_state_ == RpcState::Stopped)
    return;

  // ok means that the data/metadata/status/etc is going to go to the
  // wire. If it is false, it not going to the wire because the call
  // is already dead (i.e., canceled, deadline expired, other side
  // dropped the channel, etc).
  if (ok)
  {
    if (rpc_handler_info_.rpc_type == grpc::internal::RpcMethod::BIDI_STREAMING
     || rpc_handler_info_.rpc_type == grpc::internal::RpcMethod::SERVER_STREAMING)
    {
      execute_queue();
    }
  }
}

inline void RpcImpl::on_finish(const bool /*ok*/) noexcept
{
  // If it is false, it not going to the wire because the call is already dead
  finish_event_.set_pending(false);
  is_finished_ = true;
}

inline void RpcImpl::on_done(
  const bool /*ok*/) noexcept
{
  done_event_.set_pending(false);
  if (rpc_state_ == RpcState::Stopped)
    return;

  rpc_state_ = RpcState::Closed;

  try
  {
    handler_->on_finish();
  }
  catch (const eh::Exception& exc)
  {
    try
    {
      Stream::Error stream;
      stream << FNS
             << ": on_finish is failed. Reason: "
             << exc.what();
      logger_->error(stream.str(), Aspect::RPCIMPL);
    }
    catch (...)
    {
    }
  }
}

inline void RpcImpl::read_if_needed() noexcept
{
  if (rpc_state_ == RpcState::Closed
   || rpc_state_ == RpcState::Stopped)
    return;

  switch (rpc_handler_info_.rpc_type)
  {
    case grpc::internal::RpcMethod::BIDI_STREAMING:
    {
      server_async_reader_writer_->Read(request_.get(), &read_event_);
      read_event_.set_pending(true);
      break;
    }
    case grpc::internal::RpcMethod::CLIENT_STREAMING:
    {
      server_async_reader_->Read(request_.get(), &read_event_);
      read_event_.set_pending(true);
      break;
    }
    case grpc::internal::RpcMethod::NORMAL_RPC:
    case grpc::internal::RpcMethod::SERVER_STREAMING:
    {
      try
      {
        handler_->on_request_internal(*request_);
      }
      catch (const eh::Exception& exc)
      {
        try
        {
          Stream::Error stream;
          stream << FNS
                 << ": on_request :"
                 << exc.what();
          logger_->error(stream.str(), Aspect::RPCIMPL);
        }
        catch (...)
        {
        }

        grpc::Status status(grpc::Status::CANCELLED);
        finish(std::move(status));
      }

      try
      {
        handler_->on_reads_done();
      }
      catch (const eh::Exception& exc)
      {
        try
        {
          Stream::Error stream;
          stream << FNS
                 << ": on_reads_done :"
                 << exc.what();
          logger_->error(stream.str(), Aspect::RPCIMPL);
        }
        catch (...)
        {
        }
      }
      break;
    }
  }
}

inline void RpcImpl::try_close() noexcept
{
  if (rpc_state_ != RpcState::Closed)
    return;

  if (read_event_.is_pending()
   || write_event_.is_pending()
   || finish_event_.is_pending()
   || done_event_.is_pending())
  {
    return;
  }

  rpc_pool_.remove(this);
}

inline bool RpcImpl::write(MessagePtr&& message) noexcept
{
  try
  {
    if (thread_id_ == std::this_thread::get_id())
    {
      pending_queue_.emplace(
        PendingQueueType::Write,
        std::move(message),
        StatusOptional{});
      execute_queue();
    }
    else
    {
      auto event = std::make_unique<EventQueue>(
        weak_from_this(),
        PendingQueueData(
          PendingQueueType::Write,
          std::move(message),
          StatusOptional{}));
      auto* event_ptr = event.release();
      const bool is_success = notifier_.Notify(
        server_completion_queue_.get(),
        event_ptr);
      if (!is_success)
      {
        event.reset(event_ptr);
        return false;
      }
    }

    return true;
  }
  catch (const eh::Exception& exc)
  {
    try
    {
      Stream::Error stream;
      stream << FNS
             << ": "
             << exc.what();
      logger_->error(stream.str(), Aspect::RPCIMPL);
    }
    catch (...)
    {
    }
  }

  return false;
}

inline bool RpcImpl::finish(grpc::Status&& status) noexcept
{
  try
  {
    if (thread_id_ == std::this_thread::get_id())
    {
      pending_queue_.emplace(
        PendingQueueType::Finish,
        MessagePtr{},
        std::move(status));
      execute_queue();
    }
    else
    {
      auto event = std::make_unique<EventQueue>(
        weak_from_this(),
        PendingQueueData(
          PendingQueueType::Finish,
          MessagePtr{},
          std::move(status)));
      auto* event_ptr = event.release();
      const bool is_success = notifier_.Notify(
        server_completion_queue_.get(),
        event_ptr);
      if (!is_success)
      {
        event.reset(event_ptr);
        return false;
      }
    }

    return true;
  }
  catch (const eh::Exception& exc)
  {
    try
    {
      Stream::Error stream;
      stream << FNS
             << ": "
             << exc.what();
      logger_->error(stream.str(), Aspect::RPCIMPL);
    }
    catch (...)
    {
    }
  }

  return false;
}

inline bool RpcImpl::stop() noexcept
{
  try
  {
    auto event = std::make_unique<EventQueue>(
      weak_from_this(),
      PendingQueueData(
        PendingQueueType::Finish,
        MessagePtr{},
        grpc::Status::CANCELLED));
    auto* event_ptr = event.release();
    bool is_success = notifier_.Notify(
      server_completion_queue_.get(),
      event_ptr);
    if (!is_success)
    {
      event.reset(event_ptr);
      return false;
    }

    event = std::make_unique<EventQueue>(
      weak_from_this(),
      PendingQueueData(
        PendingQueueType::Stop,
        MessagePtr{},
        StatusOptional{}));
    event_ptr = event.release();
    is_success = notifier_.Notify(
      server_completion_queue_.get(),
      event_ptr);
    if (!is_success)
    {
      event.reset(event_ptr);
      return false;
    }

    return true;
  }
  catch (const eh::Exception& exc)
  {
    try
    {
      Stream::Error stream;
      stream << FNS
             << ": "
             << exc.what();
      logger_->error(stream.str(), Aspect::RPCIMPL);
    }
    catch (...)
    {
    }
  }

  return false;
}

inline std::weak_ptr<Rpc>
RpcImpl::get_weak_ptr() noexcept
{
  return weak_from_this();
}

inline bool RpcImpl::is_stopped() noexcept
{
  return is_stopped_.load(std::memory_order_acquire);
}

inline void RpcImpl::execute_queue() noexcept
{
  try
  {
    if (rpc_state_ == RpcState::Stopped
     || write_event_.is_pending()
     || finish_event_.is_pending())
    {
      return;
    }

    if (pending_queue_.empty())
      return;

    auto data = std::move(pending_queue_.front());
    pending_queue_.pop();

    const PendingQueueType type = std::get<0>(data);
    auto message = std::move(std::get<1>(data));
    auto status = std::move(std::get<2>(data));

    if (type == PendingQueueType::Stop)
    {
      rpc_state_ = RpcState::Stopped;
      is_stopped_.store(true, std::memory_order_release);
      return;
    }

    if (is_finished_)
      return;

    if (rpc_state_ == RpcState::Idle)
    {
      execute_queue();
      return;
    }

    if (rpc_handler_info_.rpc_type == grpc::internal::RpcMethod::BIDI_STREAMING
     || rpc_handler_info_.rpc_type == grpc::internal::RpcMethod::SERVER_STREAMING)
    {
      execute_queue_stream(type, status, std::move(message));
    }
    else
    {
      execute_unique(status, std::move(message));
    }
  }
  catch (const eh::Exception& exc)
  {
    try
    {
      Stream::Error stream;
      stream << FNS
             << ": "
             << exc.what();
      logger_->error(stream.str(), Aspect::RPCIMPL);
    }
    catch (...)
    {
    }
  }
}

inline void RpcImpl::execute_queue_stream(
  const PendingQueueType type,
  const StatusOptional& status,
  MessagePtr&& message)
{
  response_ = std::move(message);
  switch (type)
  {
    case PendingQueueType::Write:
    {
      switch (rpc_handler_info_.rpc_type)
      {
        case grpc::internal::RpcMethod::BIDI_STREAMING:
          server_async_reader_writer_->Write(*response_, &write_event_);
          write_event_.set_pending(true);
          break;
        case grpc::internal::RpcMethod::SERVER_STREAMING:
          server_async_writer_->Write(*response_, &write_event_);
          write_event_.set_pending(true);
          break;
        default:
          Stream::Error stream;
          stream << FNS
                 << ": not correct type in Queue::Type::Write";
          logger_->emergency(stream.str(), Aspect::RPCIMPL);
          return;
      }
      break;
    }
    case PendingQueueType::Finish:
    {
      if (!status)
      {
        Stream::Error stream;
        stream << FNS
               << ": status is null. Logic error";
        logger_->emergency(stream.str(), Aspect::RPCIMPL);
        return;
      }

      switch (rpc_handler_info_.rpc_type)
      {
        case grpc::internal::RpcMethod::BIDI_STREAMING:
          server_async_reader_writer_->Finish(*status, &finish_event_);
          finish_event_.set_pending(true);
          break;
        case grpc::internal::RpcMethod::SERVER_STREAMING:
          server_async_writer_->Finish(*status, &finish_event_);
          finish_event_.set_pending(true);
          break;
        default:
          Stream::Error stream;
          stream << FNS
                 << ": not correct type in Queue::Type::Finish";
          logger_->emergency(stream.str(), Aspect::RPCIMPL);
          return;
      }
      break;
    }
    default:
    {
      Stream::Error stream;
      stream << FNS
             << ": Not correct type";
      logger_->emergency(stream.str(), Aspect::RPCIMPL);
    }
  }
}

inline void RpcImpl::execute_unique(
  const StatusOptional& status,
  MessagePtr&& message)
{
  response_ = std::move(message);
  if (response_)
  {
    switch (rpc_handler_info_.rpc_type)
    {
      case grpc::internal::RpcMethod::NORMAL_RPC:
        server_async_response_writer_->Finish(
          *response_,
          status ? *status : grpc::Status::OK,
          &finish_event_);
        break;
      case grpc::internal::RpcMethod::CLIENT_STREAMING:
        server_async_reader_->Finish(
          *response_,
          status ? *status : grpc::Status::OK,
          &finish_event_);
        break;
      default:
        Stream::Error stream;
        stream << FNS
               << ": Not correct type";
        logger_->emergency(stream.str(), Aspect::RPCIMPL);
        return;
    }
  }
  else
  {
    switch (rpc_handler_info_.rpc_type)
    {
      case grpc::internal::RpcMethod::NORMAL_RPC:
        server_async_response_writer_->FinishWithError(
          status ? *status : grpc::Status::OK,
          &finish_event_);
        break;
      case grpc::internal::RpcMethod::CLIENT_STREAMING:
        server_async_reader_->FinishWithError(
          status ? *status : grpc::Status::OK,
          &finish_event_);
        break;
      default:
        Stream::Error stream;
        stream << FNS
               << ": Not correct type";
        logger_->emergency(stream.str(), Aspect::RPCIMPL);
        return;
    }
  }

  finish_event_.set_pending(true);
}

} // namespace UServerUtils::Grpc::Core::Server