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
  Logger* logger,
  Queues&& queues)
  : logger_(ReferenceCounting::add_ref(logger)),
    queues_(std::move(queues))
{
  if (queues_.empty())
  {
    Stream::Error stream;
    stream << FNS
           << ": Queues is empty";
    throw Exception(stream);
  }

  std::stringstream stream;
  stream << "Starting Scheduler with number threads = "
         << size();
  logger_->info(stream.str(), Aspect::SCHEDULER);

  try
  {
    const auto size = queues_.size();
    for (std::size_t i = 0; i < size; ++i)
    {
      auto completion_queue = queues_[i];
      auto function = [completion_queue] () {
        bool ok = false;
        void *tag = nullptr;
        while (completion_queue->Next(&tag, &ok))
        {
          auto *event = static_cast<Event *>(tag);
          event->handle(ok);
        }
      };
      threads_.add(std::move(function));
    }
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error stream;
    stream << FNS
           << ": Starting scheduler is failure: "
           << exc.what();
    throw Exception(stream);
  }

  logger_->info(
    String::SubString("Scheduler is succesfully started"),
    Aspect::SCHEDULER);
}

Scheduler::~Scheduler()
{
  try
  {
    logger_->info(
      String::SubString("Stopping Scheduler..."),
      Aspect::SCHEDULER);

    for (auto& queue : queues_)
    {
      queue->Shutdown();
    }

    threads_.clear();

    logger_->info(
      String::SubString("Scheduler is succesfully stopped"),
      Aspect::SCHEDULER);
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

std::size_t Scheduler::size() const noexcept
{
  return queues_.size();
}

const Scheduler::Queues&
Scheduler::queues() noexcept
{
  return queues_;
}

} // UServerUtils::Grpc::Core::Common
