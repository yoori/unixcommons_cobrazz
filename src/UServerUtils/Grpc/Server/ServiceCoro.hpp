#ifndef GRPC_SERVER_SERVICE_CORO_H_
#define GRPC_SERVER_SERVICE_CORO_H_

// STD
#include <memory>

// USERVER
#include <userver/concurrent/queue.hpp>

// THIS
#include <Generics/Uncopyable.hpp>
#include <Logger/Logger.hpp>
#include <UServerUtils/Grpc/Server/RpcHandlerImpl.hpp>
#include <UServerUtils/Component.hpp>

namespace UServerUtils::Grpc::Server
{

namespace Aspect
{

const char SERVICE_CORO[] = "SERVICE_CORO";

} // namespace Aspect

// [Mode[client] - Mode[server]]
enum class ReadStatus
{
  Initialize,     // Event occurs only for Stream-Stream and Stream-Unary mode. Event occur on initialize Rpc.
  Read,           // Event occurs on read
  ReadsDone,      // Event occurs only for Stream-Stream and Stream-Unary mode
  RpcFinish,      // Event occurs only for Stream-Stream and Stream-Unary mode
  Deadline,       // Event occurs when deadline is triggered
  InternalError,  // Event on internal error
  Finish          // Event indicating that the coroutine should end
};

namespace Internal
{

namespace Types
{

using IdRpc = std::uintptr_t;

} // namespace Types

template<class Request, class Response>
struct DataQueueCoro final
{
  enum class Type
  {
    Initialize,
    Read,
    ReadsDone,
    RpcFinish
  };

  using IdRpc = Types::IdRpc;
  using RequestPtr = std::unique_ptr<Request>;
  using WriterPtr = std::unique_ptr<Writer<Response>>;

  DataQueueCoro(
    const Type type = Type::Read,
    RequestPtr&& request = {},
    WriterPtr&& writer = {},
    const IdRpc id_rpc = 0)
    : type(type),
      request(std::move(request)),
      writer(std::move(writer)),
      id_rpc(id_rpc)
  {
  }

  DataQueueCoro(const DataQueueCoro&) = delete;
  DataQueueCoro(DataQueueCoro&&) = default;
  DataQueueCoro& operator=(const DataQueueCoro&) = delete;
  DataQueueCoro& operator=(DataQueueCoro&&) = default;

  Type type;
  RequestPtr request;
  WriterPtr writer;
  IdRpc id_rpc;
};

template<class Request, class Response>
using QueueCoro = userver::concurrent::GenericQueue<
  DataQueueCoro<Request, Response>,
  userver::concurrent::impl::SimpleQueuePolicy<true, true>>;

template<class Request, class Response>
class Reader final
{
public:
  using RequestPtr = std::unique_ptr<Request>;
  using WriterPtr = std::unique_ptr<Writer<Response>>;
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using Consumer = typename QueueCoro<Request, Response>::Consumer;
  using IdRpc = Types::IdRpc;

  struct Data final
  {
    static constexpr IdRpc k_empty_id_rpc = 0;

    Data(
      const ReadStatus status,
      WriterPtr&& writer,
      RequestPtr&& request,
      const IdRpc id_rpc)
      : status(status),
        writer(std::move(writer)),
        request(std::move(request)),
        id_rpc(id_rpc)
    {
    }

    ReadStatus status;
    WriterPtr writer;
    RequestPtr request;
    IdRpc id_rpc;
  };

private:
  using DataQueue = DataQueueCoro<Request, Response>;

public:
  explicit Reader(
    Logger* logger,
    Consumer&& consumer) noexcept
    : logger_(ReferenceCounting::add_ref(logger)),
      consumer_(std::move(consumer))
  {
  }

  ~Reader() = default;

  // Timeout in milliseconds (optional)
  Data read(std::optional<std::size_t> timeout = {}) const noexcept
  {
    userver::engine::Deadline deadline;
    if (timeout)
    {
      deadline = userver::engine::Deadline::FromDuration(
        std::chrono::milliseconds(*timeout));
    }

    if (is_finish_)
    {
      return {ReadStatus::Finish, {}, {}, Data::k_empty_id_rpc};
    }

    DataQueue data_queue;
    try
    {
      if (consumer_.Pop(data_queue, deadline))
      {
        const auto type = data_queue.type;
        switch (type)
        {
          case DataQueue::Type::Initialize:
            return {
              ReadStatus::Initialize,
              {},
              {},
              data_queue.id_rpc
            };
          case DataQueue::Type::Read:
            return {
              ReadStatus::Read,
              std::move(data_queue.writer),
              std::move(data_queue.request),
              data_queue.id_rpc
            };
          case DataQueue::Type::ReadsDone:
            return {
              ReadStatus::ReadsDone,
              std::move(data_queue.writer),
              {},
              data_queue.id_rpc
            };
          case DataQueue::Type::RpcFinish:
            return {
              ReadStatus::RpcFinish,
              {},
              {},
              data_queue.id_rpc
            };
          default:
            Stream::Error stream;
            stream << FNS
                   << "Unknow type="
                   << static_cast<int>(type);
            logger_->error(
              stream.str(),
              Aspect::SERVICE_CORO);
            return {
              ReadStatus::InternalError,
              std::move(data_queue.writer),
              {},
              data_queue.id_rpc
            };
        }
      }
      else
      {
        if (deadline.IsReachable())
        {
          if (deadline.IsReached())
          {
            return {ReadStatus::Deadline, {}, {}, Data::k_empty_id_rpc};
          }
          else
          {
            is_finish_ = true;
            return {ReadStatus::Finish, {}, {}, Data::k_empty_id_rpc};
          }
        }
        else
        {
          is_finish_ = true;
          return {ReadStatus::Finish, {}, {}, Data::k_empty_id_rpc};
        }
      }
    }
    catch (const std::exception& exc)
    {
      Stream::Error stream;
      stream << FNS
             << exc.what();
      logger_->error(
        stream.str(),
        Aspect::SERVICE_CORO);
    }
    catch (...)
    {
      Stream::Error stream;
      stream << FNS
             << "Unknown error";
      logger_->error(
        stream.str(),
        Aspect::SERVICE_CORO);
    }

    return {ReadStatus::InternalError, {}, {}, Data::k_empty_id_rpc};
  }

  bool is_finish() const noexcept
  {
    return is_finish_;
  }

private:
  Logger_var logger_;

  Consumer consumer_;

  mutable bool is_finish_ = false;
};

template<class Request, class Response>
using ReaderPtr = std::unique_ptr<Reader<Request, Response>>;

} // namespace Internal

template <class Request, class Response>
class ServiceCoro :  public UServerUtils::Component
{
public:
  using Reader = Internal::Reader<Request, Response>;

public:
  virtual void handle(const Reader& reader) = 0;

  virtual DefaultErrorCreatorPtr<Response>
  default_error_creator(const Request& /*request*/) noexcept
  {
    return {};
  }

protected:
  ServiceCoro() = default;

  ~ServiceCoro() override = default;
};

template<class Request, class Response>
using ServiceCoro_var = ReferenceCounting::SmartPtr<
  ServiceCoro<Request, Response>>;

} // namespace UServerUtils::Grpc::Server

#endif // GRPC_SERVER_SERVICE_CORO_H_