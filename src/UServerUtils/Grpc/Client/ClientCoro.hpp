#ifndef GRPC_CLIENT_CLIENT_CORO_H_
#define GRPC_CLIENT_CLIENT_CORO_H_

// STD
#include <exception>
#include <memory>
#include <unordered_map>

// GRPC
#include <grpcpp/grpcpp.h>
#include <grpcpp/notifier.h>

// USERVER
#include <userver/engine/future.hpp>

// THIS
#include <eh/Exception.hpp>
#include <Logger/Logger.hpp>
#include <UServerUtils/Grpc/Client/EventCoro.hpp>
#include <UServerUtils/Grpc/Client/EventObserverCoro.hpp>
#include <UServerUtils/Grpc/Client/ClientObserver.hpp>
#include <UServerUtils/Grpc/Client/Types.hpp>
#include <UServerUtils/Grpc/Client/Writer.hpp>

namespace UServerUtils::Grpc::Client
{

namespace Aspect
{

constexpr const char CLIENT_CORO[] = "CLIENT_CORO";

} // namespace Aspect

enum class Status
{
  Ok = 0,
  Timeout,
  InternalError
};

template<class RpcServiceMethodConcept, class = void>
class ClientCoro final
{
};

template<class RpcServiceMethodConcept>
class ClientCoro<
  RpcServiceMethodConcept,
  Internal::has_bidi_streaming_t<RpcServiceMethodConcept>> final
  : public ClientObserver<RpcServiceMethodConcept>,
    public EventObserverCoro<Internal::Response<RpcServiceMethodConcept>>,
    public EventObserverTimeoutCoro,
    public std::enable_shared_from_this<ClientCoro<RpcServiceMethodConcept>>
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;

  using Traits = Internal::Traits<RpcServiceMethodConcept>;
  using Request = typename Traits::Request;
  using RequestPtr =  std::unique_ptr<Request>;
  using Response = typename Traits::Response;
  using ResponsePtr = std::unique_ptr<Response>;

  static_assert(
    Internal::has_member_id_request_grpc_v<Request>,
    "Request must have member 'std::uint32_t id_request_grpc'");
  static_assert(
    Internal::has_member_id_request_grpc_v<Response>,
    "Response must have member 'std::uint32_t id_request_grpc'");

  using Observer = ClientObserver<RpcServiceMethodConcept>;
  using CompletionQueue = typename Observer::CompletionQueue;
  using ClientCoroPtr = std::shared_ptr<ClientCoro>;
  using Channel = grpc::Channel;

  struct WriteResult final
  {
    WriteResult(const Status status, ResponsePtr&& response)
      : status(status),
        response(std::move(response))
    {
    }

    ~WriteResult() = default;

    WriteResult(const WriteResult&) = delete;
    WriteResult(WriteResult&&) = default;
    WriteResult& operator=(const WriteResult&) = delete;
    WriteResult& operator=(WriteResult&&) = default;

    Status status;
    ResponsePtr response;
  };

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

private:
  using Promise = userver::engine::Promise<ResponsePtr>;
  using Future = userver::engine::Future<ResponsePtr>;
  using Requests = std::unordered_map<IdRequest, Promise>;
  using Counter = std::atomic<std::uint32_t>;

public:
  ~ClientCoro() override = default;

  static ClientCoroPtr create(Logger* logger)
  {
    return std::shared_ptr<ClientCoro>(new ClientCoro(logger));
  }

  ClientId client_id() const noexcept
  {
    return Observer::client_id();
  }

  WriteResult write(
    RequestPtr&& request,
    const std::size_t timeout = 3000) noexcept
  {
    try
    {
      if (!request)
      {
        Stream::Error stream;
        stream << FNS
               << ": request is empty";
        throw Exception(stream);
      }

      Promise promise;
      auto future = promise.get_future();

      const auto id = counter_.fetch_add(
        1,
        std::memory_order_relaxed);
      request->set_id_request_grpc(id);

      auto event = std::make_unique<EventCoro<Response>>(
        std::move(promise),
        id,
        this->weak_from_this());
      auto* event_ptr = event.release();
      const bool is_success = notifier_.Notify(
        Observer::completion_queue().get(),
        event_ptr);
      if (!is_success)
      {
        event.reset(event_ptr);
        return {Status::InternalError, {}};
      }

      const auto write_status = Observer::writer()->write(
        std::move(request));

      if (write_status != WriterStatus::Ok)
      {
        auto event_timeout = std::make_unique<EventTimeoutCoro>(
          id,
          this->weak_from_this());
        auto* event_timeout_ptr = event_timeout.release();
        const bool is_success = notifier_.Notify(
          Observer::completion_queue().get(),
          event_timeout_ptr);
        if (!is_success)
        {
          event_timeout.reset(event_timeout_ptr);
          return {Status::InternalError, {}};
        }

        Stream::Error stream;
        stream << FNS
               << "WriterStatus="
               << (write_status == WriterStatus::InternalError ?
                  "InternalError" : "RpcClosed");
        logger_->error(stream.str(), Aspect::CLIENT_CORO);
        return {Status::InternalError, {}};
      }

      ResponsePtr response;
      userver::engine::FutureStatus status =
        userver::engine::FutureStatus::kTimeout;
      try
      {
        status = future.wait_for(
          std::chrono::milliseconds(timeout));
        if (status == userver::engine::FutureStatus::kReady)
        {
          response = future.get();
        }
      }
      catch (...)
      {
        status = userver::engine::FutureStatus::kCancelled;
      }

      if (status != userver::engine::FutureStatus::kReady)
      {
        auto event_timeout = std::make_unique<EventTimeoutCoro>(
          id,
          this->weak_from_this());
        auto* event_timeout_ptr = event_timeout.release();
        const bool is_success = notifier_.Notify(
          Observer::completion_queue().get(),
          event_timeout_ptr);
        if (!is_success)
        {
          event_timeout.reset(event_timeout_ptr);
          return {Status::InternalError, {}};
        }
      }

      switch (status)
      {
        case userver::engine::FutureStatus::kReady:
        {
          if (response)
          {
            return {Status::Ok, std::move(response)};
          }
          else
          {
            Stream::Error stream;
            stream << FNS
                   << "Logic error. Empty response";
            logger_->error(stream.str(), Aspect::CLIENT_CORO);

            return {Status::InternalError, {}};
          }
        }
        case userver::engine::FutureStatus::kTimeout:
        {
          return {Status::Timeout, {}};
        }
        case userver::engine::FutureStatus::kCancelled:
        {
          return {Status::InternalError, {}};
        }
      }
    }
    catch (const eh::Exception& exc)
    {
      Stream::Error stream;
      stream << FNS
             << exc.what();
      logger_->error(stream.str(), Aspect::CLIENT_CORO);
    }
    catch (...)
    {
      Stream::Error stream;
      stream << FNS
             << "Unknown error";
      logger_->error(stream.str(), Aspect::CLIENT_CORO);
    }

    return {Status::InternalError, {}};
  }

private:
  explicit ClientCoro(Logger* logger)
    : logger_(ReferenceCounting::add_ref(logger))
  {
    requests_.reserve(100000);
  }

  void on_timeout(
    const bool /*ok*/,
    const IdRequest id_request) noexcept override
  {
    requests_.erase(id_request);
  }

  void on_event(
    const bool /*ok*/,
    Promise&& promise,
    const IdRequest id_request) noexcept override
  {
    try
    {
      if (is_stopped_)
      {
        Stream::Error stream;
        stream << FNS
               << "Client stopped";
        throw Exception(stream);
      }

      const auto result = requests_.try_emplace(
        id_request,
        std::move(promise)).second;
      if (!result)
      {
        Stream::Error stream;
        stream << FNS
               << "Logic error. Existing id_request="
               << id_request;
        throw Exception(stream);
      }
    }
    catch (const eh::Exception& exc)
    {
      Stream::Error stream;
      stream << FNS
             << exc.what();
      logger_->error(
        stream.str(),
        Aspect::CLIENT_CORO);

      auto eptr = std::current_exception();
      try
      {
        promise.set_exception(eptr);
      }
      catch (...)
      {
      }
    }
    catch (...)
    {
      Stream::Error stream;
      stream << FNS
             << "Unknown error";
      logger_->error(
        stream.str(),
        Aspect::CLIENT_CORO);

      auto eptr = std::current_exception();
      try
      {
        promise.set_exception(eptr);
      }
      catch (...)
      {
      }
    }
  }

  void on_initialize(const bool /*ok*/) override
  {
  }

  void on_read(Response&& response) override
  {
    const auto id_request = response.id_request_grpc();
    const auto it = requests_.find(id_request);
    if (it != std::end(requests_))
    {
      auto& promise = it->second;
      try
      {
        promise.set_value(
          std::make_unique<Response>(
            std::move(response)));
      }
      catch (...)
      {
      }

      requests_.erase(it);
    }
  }

  void on_finish(grpc::Status&& status) override
  {
    is_stopped_ = true;
    Exception exc(status.error_message());
    auto exception_ptr = std::make_exception_ptr(exc);
    for (auto& [id_request, promise] : requests_)
    {
      try
      {
        promise.set_exception(exception_ptr);
      }
      catch (...)
      {
      }
    }
    requests_.clear();
  }

private:
  template<class, class>
  friend class FactoryCoro;

  Logger_var logger_;

  Requests requests_;

  Counter counter_{0};

  grpc::Notifier notifier_;

  bool is_stopped_ = false;
};

template<class RpcServiceMethodConcept>
using ClientCoroPtr = std::shared_ptr<ClientCoro<RpcServiceMethodConcept>>;

} // namespace UServerUtils::Grpc::Client

#endif // GRPC_CLIENT_CLIENT_CORO_H_