#ifndef GRPC_CORE_CLIENT_EVENT_OBSERVER_CORO_H_
#define GRPC_CORE_CLIENT_EVENT_OBSERVER_CORO_H_

// USERVER
#include <userver/engine/future.hpp>

// THIS
#include <UServerUtils/Grpc/Core/Client/Types.hpp>

namespace UServerUtils::Grpc::Core::Client
{

template<class Response>
class EventObserverCoro
{
public:
  using ResponsePtr = std::unique_ptr<Response>;
  using Promise = userver::engine::Promise<ResponsePtr>;

public:
  virtual void on_event(
    const bool ok,
    Promise&& promise,
    const IdRequest id_request) noexcept = 0;

protected:
  EventObserverCoro() = default;

  virtual ~EventObserverCoro() = default;
};

class EventObserverTimeoutCoro
{
public:
  virtual void on_timeout(
    const bool ok,
    const IdRequest id_request) noexcept = 0;

protected:
  EventObserverTimeoutCoro() = default;

  virtual ~EventObserverTimeoutCoro() = default;
};

} // namespace UServerUtils::Grpc::Core::Client

#endif // GRPC_CORE_CLIENT_EVENT_OBSERVER_CORO_H_
