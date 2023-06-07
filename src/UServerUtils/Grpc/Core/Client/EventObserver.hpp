#ifndef GRPC_CORE_CLIENT_EVENT_OBSERVER_H_
#define GRPC_CORE_CLIENT_EVENT_OBSERVER_H_

// THIS
#include <UServerUtils/Grpc/Core/Client/EventType.hpp>
#include <UServerUtils/Grpc/Core/Client/PendingQueue.hpp>

namespace UServerUtils::Grpc::Core::Client
{

class EventObserver {
public:
  virtual void on_event(
    const bool ok,
    const EventType type) noexcept = 0;

protected:
  EventObserver() = default;

  virtual ~EventObserver() = default;
};

class EventQueueObserver {
public:
  virtual void on_event_queue(
    const bool ok,
    PendingQueueData&& data) noexcept = 0;

protected:
  EventQueueObserver() = default;

  virtual ~EventQueueObserver() = default;
};

} // namespace UServerUtils::Grpc::Core::Client

#endif // GRPC_CORE_CLIENT_EVENT_OBSERVER_H_
