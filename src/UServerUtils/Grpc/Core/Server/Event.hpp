#ifndef GRPC_CORE_SERVER_EVENT_H_
#define GRPC_CORE_SERVER_EVENT_H_

// STD
#include <memory>

// THIS
#include "Common/Event.hpp"
#include "EventObserver.hpp"
#include "EventType.hpp"

namespace UServerUtils::Grpc::Core::Server
{

class Event final : public Common::Event
{
public:
  using Observer = EventObserver;

public:
  Event(
    const EventType type,
    Observer& observer,
    const bool is_pending)
    : type_(type),
      observer_(observer),
      is_pending_(is_pending)
  {
  }

  ~Event() override = default;

  void handle(const bool ok) noexcept override
  {
    observer_.on_event(ok, type_);
  }

  void set_pending(const bool is_pending) noexcept
  {
    is_pending_ = is_pending;
  }

  bool is_pending() const noexcept
  {
    return is_pending_;
  }

private:
  const EventType type_;

  Observer& observer_;

  bool is_pending_ = false;
};

class EventQueue final : public Common::Event
{
public:
  using ObserverPtr = std::weak_ptr<EventQueueObserver>;

public:
  EventQueue(
    const ObserverPtr& observer,
    PendingQueueData&& data)
    : observer_(observer),
      data_(std::move(data))
  {
  }

  ~EventQueue() override = default;

  void handle(const bool ok) noexcept override
  {
    try
    {
      if (auto ptr = observer_.lock())
      {
        ptr->on_event_queue(ok, std::move(data_));
      }
    }
    catch (...)
    {
    }

    delete this;
  }

private:
  const ObserverPtr observer_;

  PendingQueueData data_;
};

} // namespace UServerUtils::Grpc::Core::Server

#endif //GRPC_CORE_SERVER_EVENT_H_