// THIS
#include <UServerUtils/Grpc/Core/Client/ClientImpl.hpp>

namespace UServerUtils::Grpc::Core::Client
{

namespace Aspect
{

constexpr const char CLIENT_IMPL[] = "CLIENT_IMPL";

} // namespace Aspect

template<class RpcServiceMethodConcept>
inline typename ClientImpl<RpcServiceMethodConcept>::ClientPtr
ClientImpl<RpcServiceMethodConcept>::create(
  Logger* logger,
  const ChannelPtr& channel,
  const CompletionQueuePtr& completion_queue,
  Delegate& delegate,
  Observer& observer,
  RequestPtr&& request)
{
  auto client =
    std::shared_ptr<ClientImpl<RpcServiceMethodConcept>>(
      new ClientImpl<RpcServiceMethodConcept>(
        logger,
        channel,
        completion_queue,
        delegate,
        observer,
        std::move(request)));

  auto writer = std::make_unique<Writer<Request, k_rpc_type>>(
    completion_queue,
    std::weak_ptr<ClientImpl<RpcServiceMethodConcept>>(client),
    client->get_id());

  try
  {
    if constexpr (k_rpc_type == Internal::RpcType::BIDI_STREAMING
               || k_rpc_type == Internal::RpcType::CLIENT_STREAMING)
    {
      observer.on_writer(std::move(writer));
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
      logger->error(stream.str(), Aspect::CLIENT_IMPL);
    }
    catch (...)
    {
    }
  }
  catch (...)
  {
    try
    {
      Stream::Error stream;
      stream << FNS
             << ": Unknow error";
      logger->error(stream.str(), Aspect::CLIENT_IMPL);
    }
    catch (...)
    {
    }
  }

  return client;
}

template<class RpcServiceMethodConcept>
inline ClientImpl<RpcServiceMethodConcept>::ClientImpl(
  Logger* logger,
  const ChannelPtr& channel,
  const CompletionQueuePtr& completion_queue,
  Delegate& delegate,
  Observer& observer,
  RequestPtr&& request)
  : client_id_(create_id()),
    logger_(ReferenceCounting::add_ref(logger)),
    channel_(channel),
    completion_queue_(completion_queue),
    delegate_(delegate),
    observer_(observer),
    request_(std::move(request)),
    responce_(std::make_unique<Response>()),
    rpc_method_(Traits::method_name(), Traits::rpc_type, channel_),
    initialize_event_(EventType::Initialize, *this, false),
    read_event_(EventType::Read, *this, false),
    write_event_(EventType::Write, *this, false),
    finish_event_(EventType::Finish, *this, false)
{
  if constexpr (k_rpc_type == Internal::RpcType::NORMAL_RPC
             || k_rpc_type == Internal::RpcType::SERVER_STREAMING)
  {
    if (!request_)
    {
      Stream::Error stream;
      stream << FNS
             << ": request is null";
      throw Exception(stream);
    }
  }
}

template<class RpcServiceMethodConcept>
inline ClientId
ClientImpl<RpcServiceMethodConcept>::create_id() noexcept
{
  static std::atomic<ClientId> counter{0};
  const auto id = counter.fetch_add(
    1,
    std::memory_order_relaxed);
  return id;
}

template<class RpcServiceMethodConcept>
inline void
ClientImpl<RpcServiceMethodConcept>::start() noexcept
{
  try
  {
    auto event = std::make_unique<EventStart>(
      this->weak_from_this());
    auto* event_ptr = event.release();
    const bool is_success = notifier_.Notify(
      completion_queue_.get(),
      event_ptr);
    if (!is_success)
    {
      event.reset(event_ptr);
      delegate_.need_remove(get_id());
    }

    return;
  }
  catch (const eh::Exception& exc)
  {
    try
    {
      Stream::Error stream;
      stream << FNS
             << ": "
             << exc.what();
      logger_->error(stream.str(), Aspect::CLIENT_IMPL);
    }
    catch(...)
    {
    }

    delegate_.need_remove(get_id());
  }
  catch (...)
  {
    try
    {
      Stream::Error stream;
      stream << FNS
             << ": Unknown error";
      logger_->error(stream.str(), Aspect::CLIENT_IMPL);
    }
    catch(...)
    {
    }

    delegate_.need_remove(get_id());
  }
}

template<class RpcServiceMethodConcept>
inline bool
ClientImpl<RpcServiceMethodConcept>::write(
  RequestPtr&& request) noexcept
{
  if constexpr (k_rpc_type == grpc::internal::RpcMethod::NORMAL_RPC
             || k_rpc_type == grpc::internal::RpcMethod::SERVER_STREAMING)
  {
    Stream::Error stream;
    stream << FNS
           << ": Logic error... write can be call"
           << " only for CLIENT_STREAMING or BIDI_STREAMING";
    logger_->error(stream.str(), Aspect::CLIENT_IMPL);

    return false;
  }

  try
  {
    auto event = std::make_unique<EventQueue>(
      this->weak_from_this(),
      PendingQueueData(PendingQueueType::Write, std::move(request)));
    auto* event_ptr = event.release();
    const bool is_success = notifier_.Notify(
      completion_queue_.get(),
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
      logger_->error(stream.str(), Aspect::CLIENT_IMPL);
    }
    catch (...)
    {
    }
  }

  return false;
}

template<class RpcServiceMethodConcept>
inline bool
ClientImpl<RpcServiceMethodConcept>::writes_done() noexcept
{
  if constexpr (k_rpc_type == grpc::internal::RpcMethod::NORMAL_RPC
             || k_rpc_type == grpc::internal::RpcMethod::SERVER_STREAMING)
  {
    Stream::Error stream;
    stream << FNS
           << ": Logic error... writes_done can be call"
           << " only for CLIENT_STREAMING or BIDI_STREAMING";
    logger_->error(stream.str(), Aspect::CLIENT_IMPL);
    return false;
  }

  try
  {
    auto event = std::make_unique<EventQueue>(
      this->weak_from_this(),
      PendingQueueData(PendingQueueType::WritesDone, MessagePtr{}));
    auto* event_ptr = event.release();
    const bool is_success = notifier_.Notify(
      completion_queue_.get(),
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
      logger_->error(stream.str(), Aspect::CLIENT_IMPL);
    }
    catch (...)
    {
    }
  }

  return false;
}

template<class RpcServiceMethodConcept>
inline ClientId
ClientImpl<RpcServiceMethodConcept>::get_id() const noexcept
{
  return client_id_;
}

template<class RpcServiceMethodConcept>
inline bool
ClientImpl<RpcServiceMethodConcept>::stop(
  std::promise<void>&& promise) noexcept
{
  try
  {
    auto event = std::make_unique<EventStop>(
      this->weak_from_this(),
      std::move(promise));
    auto* event_ptr = event.release();
    const bool is_success = notifier_.Notify(
      completion_queue_.get(),
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
      logger_->error(stream.str(), Aspect::CLIENT_IMPL);
    }
    catch (...)
    {
    }
  }

  return false;
}

template<class RpcServiceMethodConcept>
inline void
ClientImpl<RpcServiceMethodConcept>::on_start(
  const bool /*ok*/) noexcept
{
  using ClientAsyncReaderWriterFactory =
    grpc::internal::ClientAsyncReaderWriterFactory<
      google::protobuf::Message,
      google::protobuf::Message>;

  using ClinetAsyncWriterFactory =
    grpc::internal::ClientAsyncWriterFactory<
      google::protobuf::Message>;

  using ClientAsyncReaderFactory =
    grpc::internal::ClientAsyncReaderFactory<
      google::protobuf::Message>;

  using ClientAsyncResponseReaderFactory =
    grpc::internal::ClientAsyncResponseReaderFactory<
      google::protobuf::Message>;

  if (rpc_state_ == RpcState::Stop)
    return;

  if (rpc_state_ != RpcState::Idle
   || initialize_event_.is_pending())
  {
    try
    {
      Stream::Error stream;
      stream << FNS
             << ": Logic error. "
             << "Rpc_state must be Idle and"
                " initialize_event can't be pending";
      logger_->error(stream.str(), Aspect::CLIENT_IMPL);
    }
    catch (...)
    {
    }

    return;
  }

  try
  {
    if constexpr (k_rpc_type == grpc::internal::RpcMethod::BIDI_STREAMING)
    {
      client_reader_writer_ =
        std::unique_ptr<ClientAsyncReaderWriter>(
          ClientAsyncReaderWriterFactory::Create(
            channel_.get(),
            completion_queue_.get(),
            rpc_method_,
            &client_context_,
            true,
            &initialize_event_));
    }
    else if constexpr (k_rpc_type == grpc::internal::RpcMethod::CLIENT_STREAMING)
    {
      client_writer_ =
        std::unique_ptr<ClientAsyncWriter>(
          ClinetAsyncWriterFactory::Create(
            channel_.get(),
            completion_queue_.get(),
            rpc_method_,
            &client_context_,
            responce_.get(),
            true,
            &initialize_event_));
    }
    else if constexpr (k_rpc_type == grpc::internal::RpcMethod::SERVER_STREAMING)
    {
      client_reader_ =
        std::unique_ptr<ClientAsyncReader>(
          ClientAsyncReaderFactory::Create(
            channel_.get(),
            completion_queue_.get(),
            rpc_method_,
            &client_context_,
            *request_,
            true,
            &initialize_event_));
    }
    else if constexpr (k_rpc_type == grpc::internal::RpcMethod::NORMAL_RPC)
    {
      client_response_reader_ =
        std::unique_ptr<ClientAsyncResponseReader>(
          ClientAsyncResponseReaderFactory::Create<google::protobuf::Message>(
            channel_.get(),
            completion_queue_.get(),
            rpc_method_,
            &client_context_,
            *request_,
            true));
      on_initialize(true);
      return;
    }
    else
    {
      Stream::Error stream;
      stream << FNS
             << ": Unknown rpc type";
      throw Exception(stream);
    }

    initialize_event_.set_pending(true);
  }
  catch (const eh::Exception& exc)
  {
    on_error(exc.what());
  }
}

template<class RpcServiceMethodConcept>
inline void
ClientImpl<RpcServiceMethodConcept>::on_initialize(
  const bool ok) noexcept
{
  initialize_event_.set_pending(false);

  if (rpc_state_ == RpcState::Stop)
    return;

  if (rpc_state_ != RpcState::Idle)
  {
    Stream::Error stream;
    stream << FNS
           << ": Logic error. Rpc state must be Idle";
    logger_->error(stream.str(), Aspect::CLIENT_IMPL);
    return;
  }

  // Client-side StartCall/RPC invocation: ok indicates that the RPC is
  // going to go to the wire. If it is false, it not going to the wire. This
  // would happen if the channel is either permanently broken or
  // transiently broken but with the fail-fast option.
  try
  {
    if constexpr (k_rpc_type != Internal::RpcType::NORMAL_RPC)
    {
      observer_.on_initialize(ok);
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
      logger_->error(stream.str(), Aspect::CLIENT_IMPL);
    }
    catch (...)
    {
    }
  }
  catch (...)
  {
    try
    {
      Stream::Error stream;
      stream << FNS
             << ": Unknown error";
      logger_->error(stream.str(), Aspect::CLIENT_IMPL);
    }
    catch (...)
    {
    }
  }

  try
  {
    if (ok)
    {
      rpc_state_ = RpcState::Initialize;

      if constexpr (k_rpc_type == grpc::internal::RpcMethod::NORMAL_RPC)
      {
        client_response_reader_->Finish(
          responce_.get(),
          &status_,
          &finish_event_);
      }
      else if constexpr (k_rpc_type == grpc::internal::RpcMethod::SERVER_STREAMING)
      {
        client_reader_->Finish(
          &status_,
          &finish_event_);
      }
      else if constexpr (k_rpc_type == grpc::internal::RpcMethod::CLIENT_STREAMING)
      {
        client_writer_->Finish(
          &status_,
          &finish_event_);
      }
      else if constexpr (k_rpc_type == grpc::internal::RpcMethod::BIDI_STREAMING)
      {
        client_reader_writer_->Finish(
          &status_,
          &finish_event_);
      }

      finish_event_.set_pending(true);

      if constexpr (k_rpc_type == grpc::internal::RpcMethod::BIDI_STREAMING
                 || k_rpc_type == grpc::internal::RpcMethod::CLIENT_STREAMING)
      {
        execute_queue();
      }

      if constexpr(k_rpc_type == grpc::internal::RpcMethod::BIDI_STREAMING
                || k_rpc_type == grpc::internal::RpcMethod::SERVER_STREAMING)
      {
        request_new_read();
      }
    }
    else
    {
      Stream::Error stream;
      stream << FNS
             << "The channel is either permanently broken or "
             << "transiently broken but with the fail-fast option";
      logger_->error(stream.str(), Aspect::CLIENT_IMPL);

      grpc::Status status(
        grpc::StatusCode::UNAVAILABLE,
        "Channel error",
        "The channel is either permanently broken or transiently"
        " broken but with the fail-fast option");

      on_finish(true);
    }
  }
  catch (const eh::Exception& exc)
  {
    on_error(exc.what());
  }
}

template<class RpcServiceMethodConcept>
inline void
ClientImpl<RpcServiceMethodConcept>::on_read(
  const bool ok) noexcept
{
  read_event_.set_pending(false);

  // ok indicates whether there is a valid message that got read
  // ok == false - this only happens because the call is dead
  if constexpr (k_rpc_type == Internal::RpcType::BIDI_STREAMING
             || k_rpc_type == Internal::RpcType::SERVER_STREAMING)
  {
    if (rpc_state_ != RpcState::Initialize
     && rpc_state_ != RpcState::WritesDone
     || !ok)
    {
      return;
    }

    try
    {
      observer_.on_read(static_cast<Response&&>(*responce_));
    }
    catch (const eh::Exception &exc)
    {
      try
      {
        Stream::Error stream;
        stream << FNS
               << ": "
               << exc.what();
        logger_->error(
          stream.str(),
          Aspect::CLIENT_IMPL);
      }
      catch (...)
      {
      }
    }
    catch (...)
    {
      try
      {
        Stream::Error stream;
        stream << FNS
               << ": unknown error";
        logger_->error(
          stream.str(),
          Aspect::CLIENT_IMPL);
      }
      catch (...)
      {
      }
    }

    request_new_read();
  }
}

template<class RpcServiceMethodConcept>
inline void
ClientImpl<RpcServiceMethodConcept>::on_write(
  const bool ok) noexcept
{
  write_event_.set_pending(false);

  // ok means that the data/metadata/status/etc is going to go to the
  // wire. If it is false, it not going to the wire because the call
  // is already dead
  if (rpc_state_ != RpcState::Initialize
  || !ok)
  {
    return;
  }

  execute_queue();
}

template<class RpcServiceMethodConcept>
inline void
ClientImpl<RpcServiceMethodConcept>::on_error(
  const std::string_view info) noexcept
{
  client_context_.TryCancel();

  if (rpc_state_ == RpcState::Finish
   || rpc_state_ == RpcState::Error)
  {
    try_close();
    return;
  }

  rpc_state_ = RpcState::Error;

  try
  {
    Stream::Error stream;
    stream << FNS
           << ": "
           << info;
    logger_->error(stream.str(), Aspect::CLIENT_IMPL);
  }
  catch (...)
  {
  }
}

template<class RpcServiceMethodConcept>
inline void
ClientImpl<RpcServiceMethodConcept>::on_finish(
  const bool /*ok*/) noexcept
{
  finish_event_.set_pending(false);

  if (rpc_state_ == RpcState::Finish)
    return;

  // Client-side Finish: \a ok should always be true
  rpc_state_ = RpcState::Finish;
  try
  {
    if constexpr (k_rpc_type == Internal::RpcType::BIDI_STREAMING
               || k_rpc_type == Internal::RpcType::SERVER_STREAMING)
    {
      observer_.on_finish(std::move(status_));
    }
    else
    {
      observer_.on_finish(
        std::move(status_),
        static_cast<Response&&>(*responce_));
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
      logger_->error(stream.str(), Aspect::CLIENT_IMPL);
    }
    catch (...)
    {
    }
  }
  catch (...)
  {
    try
    {
      Stream::Error stream;
      stream << FNS
             << ": Unknown error";
      logger_->error(stream.str(), Aspect::CLIENT_IMPL);
    }
    catch (...)
    {
    }
  }
}

template<class RpcServiceMethodConcept>
inline void
ClientImpl<RpcServiceMethodConcept>::on_stop(
  const bool /*ok*/) noexcept
{
  rpc_state_ = RpcState::Stop;
  client_context_.TryCancel();
}

template<class RpcServiceMethodConcept>
inline void
ClientImpl<RpcServiceMethodConcept>::on_event(
  const bool ok,
  const EventType type) noexcept
{
  switch (type)
  {
    case EventType::Start:
    {
      on_start(ok);
      break;
    }
    case EventType::Initialize:
    {
      on_initialize(ok);
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
    case EventType::Stop:
    {
      on_stop(ok);
      break;
    }
  }

  try_close();
}

template<class RpcServiceMethodConcept>
inline void ClientImpl<RpcServiceMethodConcept>::on_event_queue(
  const bool ok,
  PendingQueueData&& data) noexcept
{
  if (rpc_state_ == RpcState::Idle
   || rpc_state_ == RpcState::Initialize)
  {
    try
    {
      pending_queue_.emplace(std::move(data));
    }
    catch (...)
    {
    }
  }

  if (rpc_state_ != RpcState::Initialize)
    return;

  execute_queue();
}

template<class RpcServiceMethodConcept>
inline void
ClientImpl<RpcServiceMethodConcept>::request_new_read() noexcept
{
  try
  {
    if (read_event_.is_pending())
    {
      Stream::Error stream;
      stream << FNS
             << ": Logic error. Read_event is pending";
      logger_->error(stream.str(), Aspect::CLIENT_IMPL);
      return;
    }

    if (rpc_state_ != RpcState::Initialize
     && rpc_state_ != RpcState::WritesDone)
    {
      return;
    }

    if constexpr (k_rpc_type == grpc::internal::RpcMethod::BIDI_STREAMING)
    {
      client_reader_writer_->Read(responce_.get(), &read_event_);
      read_event_.set_pending(true);
    }
    else if constexpr (k_rpc_type == grpc::internal::RpcMethod::SERVER_STREAMING)
    {
      client_reader_->Read(responce_.get(), &read_event_);
      read_event_.set_pending(true);
    }
  }
  catch(const eh::Exception& exc)
  {
    on_error(exc.what());
  }
}

template<class RpcServiceMethodConcept>
inline void
ClientImpl<RpcServiceMethodConcept>::execute_queue() noexcept
{
  if (rpc_state_ != RpcState::Initialize
   || write_event_.is_pending())
  {
    return;
  }

  if constexpr (k_rpc_type == grpc::internal::RpcMethod::NORMAL_RPC
             || k_rpc_type == grpc::internal::RpcMethod::SERVER_STREAMING)
  {
    Stream::Error stream;
    stream << FNS
           << ": Logic error... execute_queue can be call"
           << " only for CLIENT_STREAMING or BIDI_STREAMING";
    logger_->error(stream.str(), Aspect::CLIENT_IMPL);
    return;
  }

  try
  {
    if (pending_queue_.empty())
      return;

    auto data = std::move(pending_queue_.front());
    pending_queue_.pop();

    if (data.first == PendingQueueType::Write)
    {
      request_ = std::move(data.second);
      if constexpr (k_rpc_type == Internal::RpcType::BIDI_STREAMING)
      {
        client_reader_writer_->Write(*request_, &write_event_);
      }
      else if constexpr (k_rpc_type == Internal::RpcType::CLIENT_STREAMING)
      {
        client_writer_->Write(*request_, &write_event_);
      }
    }
    else if (data.first == PendingQueueType::WritesDone)
    {
      if constexpr (k_rpc_type == Internal::RpcType::BIDI_STREAMING)
      {
        client_reader_writer_->WritesDone(&write_event_);
      }
      else if constexpr (k_rpc_type == Internal::RpcType::CLIENT_STREAMING)
      {
        client_writer_->WritesDone(&write_event_);
      }

      rpc_state_ = RpcState::WritesDone;
    }
    else
    {
      Stream::Error stream;
      stream << FNS
             << ": Logic error. Not correct type message";
      logger_->error(stream.str(), Aspect::CLIENT_IMPL);
      return;
    }

    write_event_.set_pending(true);
  }
  catch (const eh::Exception& exc)
  {
    on_error(exc.what());
  }
}

template<class RpcServiceMethodConcept>
inline void
ClientImpl<RpcServiceMethodConcept>::try_close() noexcept
{
  if (rpc_state_ != RpcState::Finish)
    return;

  if (finish_event_.is_pending()
   || initialize_event_.is_pending()
   || read_event_.is_pending()
   || write_event_.is_pending())
  {
    return;
  }

  delegate_.need_remove(get_id());
}

} // namespace UServerUtils::Grpc::Core::Client