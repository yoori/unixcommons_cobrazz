// THIS
#include <UServerUtils/Grpc/Core/Common/Event.hpp>
#include <UServerUtils/Grpc/Core/Common/Scheduler.hpp>

// STD
#include <future>
#include <sstream>

namespace UServerUtils::Grpc::Core::Common
{

namespace Aspect
{

const char SCHEDULER[] = "SCHEDULER";

} // namespace Aspect

Scheduler::Scheduler(
  const Logger_var& logger,
  Queues&& queues)
  : logger_(logger),
    queues_(std::move(queues))
{
  if (queues_.empty())
  {
    Stream::Error stream;
    stream << FNS
           << ": Queues is empty";
    throw Exception(stream);
  }
}

Scheduler::~Scheduler()
{
  try
  {
    Stream::Error stream;
    bool error = false;

    if (state_ == AS_ACTIVE)
    {
      stream << FNS
             << ": wasn't deactivated.";
      error = true;

      for (auto& queue : queues_)
      {
        queue->Shutdown();
      }
    }

    if (state_ != AS_NOT_ACTIVE)
    {
      if (error)
      {
        stream << std::endl;
      }
      stream << FNS
             << ": error deactivation, still active.";
      error = true;
    }

    if (error)
    {
      logger_->error(stream.str(), Aspect::SCHEDULER);
    }
  }
  catch (const eh::Exception& exc)
  {
    try
    {
      std::cerr << FNS
                << ": eh::Exception: "
                << exc.what()
                << std::endl;
    }
    catch (...)
    {
    }
  }
}

void Scheduler::activate_object()
{
  std::stringstream stream;
  stream << "Starting Scheduler with number threads = "
         << size();
  logger_->info(stream.str(), Aspect::SCHEDULER);

  std::unique_lock lock(mutex_);
  if (state_ != AS_NOT_ACTIVE)
  {
    Stream::Error stream;
    stream << FNS
           << ": already active";
    throw ActiveObject::AlreadyActive(stream);
  }

  try
  {
    const auto size = queues_.size();
    for (std::size_t i = 0; i < size; ++i)
    {
      auto completion_queue = queues_[i];
      auto function = [completion_queue] () {
        bool ok = false;
        void* tag = nullptr;
        while(completion_queue->Next(&tag, &ok))
        {
          auto* event = static_cast<Event*>(tag);
          event->handle(ok);
        }
      };

      threads_.add(std::move(function));
    }

    state_ = AS_ACTIVE;
    lock.unlock();
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error stream;
    stream << FNS
           << ": activate_object failure: "
           << exc.what();
    throw Exception(stream);
  }

  logger_->info(
    std::string("Scheduler is succesfully started"),
    Aspect::SCHEDULER);
}

void Scheduler::deactivate_object()
{
  std::unique_lock lock(mutex_);
  if (state_ == AS_ACTIVE)
  {
    logger_->info(
      std::string("Stopping Scheduler..."),
      Aspect::SCHEDULER);

    for (auto& queue : queues_)
    {
      queue->Shutdown();
    }

    threads_.clear();

    state_ = AS_DEACTIVATING;
    lock.unlock();

    condition_variable_.notify_all();

    logger_->info(
      std::string("Scheduler is succesfully stopped"),
      Aspect::SCHEDULER);
  }
}

void Scheduler::wait_object()
{
  std::unique_lock lock(mutex_);
  condition_variable_.wait(lock, [this] () {
    return state_ != AS_ACTIVE;
  });

  if (state_ == AS_DEACTIVATING)
  {
    state_ = AS_NOT_ACTIVE;
  }
}

bool Scheduler::active()
{
  std::lock_guard lock(mutex_);
  return state_ == AS_ACTIVE;
}

std::size_t Scheduler::size() const noexcept
{
  return queues_.size();
}

} // UServerUtils::Grpc::Core::Common
