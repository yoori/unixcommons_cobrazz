#ifndef GRPC_CORE_CLIENT_EVENT_H_
#define GRPC_CORE_CLIENT_EVENT_H_

// STD
#include <future>
#include <memory>

// THIS
#include <UServerUtils/Grpc/Core/Common/Event.hpp>
#include <UServerUtils/Grpc/Core/Client/EventObserver.hpp>
#include <UServerUtils/Grpc/Core/Client/PendingQueue.hpp>

namespace UServerUtils::Grpc::Core::Client
{

class Event final : public Common::Event
{
public:
  Event(
    const EventType type,
    EventObserver& observer,
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

  EventObserver& observer_;

  bool is_pending_ = false;
};

class EventStart final : public Common::Event
{
public:
  using EventObserverPtr = std::weak_ptr<EventObserver>;

public:
  EventStart(const EventObserverPtr& observer)
    : observer_(observer)
  {
  }

  ~EventStart() override = default;

  void handle(const bool ok) noexcept override
  {
    try
    {
      if (auto ptr = observer_.lock())
      {
        ptr->on_event(ok, type_);
      }
    }
    catch (...)
    {
    }

    delete this;
  }

private:
  const EventType type_ = EventType::Start;

  const EventObserverPtr observer_;
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

class EventStop final : public Common::Event
{
public:
  using ObserverPtr = std::weak_ptr<EventObserver>;
  using StoppedPromise = std::promise<void>;

public:
  EventStop(
    const ObserverPtr& observer)
    : observer_(observer)
  {
  }

  ~EventStop() override = default;

  void handle(const bool ok) noexcept override
  {
    try
    {
      if (auto ptr = observer_.lock())
      {
        ptr->on_event(ok, type_);
      }
    }
    catch (...)
    {
    }

    delete this;
  }

private:
  const EventType type_ = EventType::Stop;

  const ObserverPtr observer_;
};

} // namespace UServerUtils::Grpc::Core::Client

#endif // GRPC_CORE_CLIENT_EVENT_H_