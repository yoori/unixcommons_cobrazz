#ifndef GRPC_CORE_CLIENT_EVENT_CORO_H_
#define GRPC_CORE_CLIENT_EVENT_CORO_H_

// STD
#include <memory>

// THIS
#include "EventObserverCoro.hpp"
#include "Common/Event.hpp"

namespace UServerUtils::Grpc::Core::Client
{

template<class Response>
class EventCoro final : public Common::Event
{
public:
  using Observer = EventObserverCoro<Response>;
  using ObserverPtr = std::weak_ptr<Observer>;
  using Promise = typename Observer::Promise;

public:
  EventCoro(
    Promise&& promise,
    const IdRequest id_request,
    const ObserverPtr& observer)
    : promise_(std::move(promise)),
      id_request_(id_request),
      observer_(observer)
  {
  }

  ~EventCoro() override = default;

  void handle(const bool ok) noexcept override
  {
    try
    {
      if (auto ptr = observer_.lock())
      {
        ptr->on_event(
          ok,
          std::move(promise_),
          id_request_);
      }
    }
    catch (...)
    {
    }

    delete this;
  }

private:
  Promise promise_;

  const IdRequest id_request_;

  const ObserverPtr observer_;
};

class EventTimeoutCoro final : public Common::Event
{
public:
  using Observer = EventObserverTimeoutCoro;
  using ObserverPtr = std::weak_ptr<Observer>;

public:
  EventTimeoutCoro(
    const IdRequest id_request,
    const ObserverPtr& observer)
    : id_request_(id_request),
      observer_(observer)
  {
  }

  ~EventTimeoutCoro() override = default;

  void handle(const bool ok)  noexcept override
  {
    try
    {
      if (auto ptr = observer_.lock())
      {
        ptr->on_timeout(ok, id_request_);
      }
    }
    catch (...)
    {
    }

    delete this;
  }

private:
  const IdRequest id_request_;

  const ObserverPtr observer_;
};

} // namespace UServerUtils::Grpc::Core::Client

#endif // GRPC_CORE_CLIENT_EVENT_CORO_H_