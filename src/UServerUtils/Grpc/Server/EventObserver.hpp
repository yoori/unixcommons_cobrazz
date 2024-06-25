#ifndef GRPC_SERVER_EVENT_OBSERVER_H_
#define GRPC_SERVER_EVENT_OBSERVER_H_

// THIS
#include <UServerUtils/Grpc/Server/EventType.hpp>
#include <UServerUtils/Grpc/Server/PendingQueue.hpp>

namespace UServerUtils::Grpc::Server
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

} // UServerUtils::Grpc::Server

#endif //GRPC_SERVER_EVENT_OBSERVER_H_