// THIS
#include "CommonContextCoro.hpp"

namespace UServerUtils::Grpc::Core::Server
{

namespace Aspect
{

const char* COMMON_CONTEXT_CORO = "COMMON_CONTEXT_CORO";

} // namespace Aspect

CommonContextCoro::CommonContextCoro(
  const Logger_var& logger,
  const MaxSizeQueue max_size_queue)
  : logger_(logger),
    max_size_queue_(max_size_queue)
{
}

CommonContextCoro::~CommonContextCoro()
{
  try
  {
    Stream::Error stream;
    bool error = false;

    if (state_ == AS_ACTIVE)
    {
      stream << FNS
             << "wasn't deactivated.";
      error = true;

      producers_.clear();

      for (auto& task: worker_tasks_)
      {
        try
        {
          task.Get();
        }
        catch (...)
        {
        }
      }
    }

    if (state_ != AS_NOT_ACTIVE)
    {
      if (error)
      {
        stream << std::endl;
      }
      stream << FNS
             << "didn't wait for deactivation, still active.";
      error = true;
    }

    if (error)
    {
      logger_->error(stream.str(), Aspect::COMMON_CONTEXT_CORO);
    }
  }
  catch (const eh::Exception& exc)
  {
    try
    {
      std::cerr << FNS
                << "eh::Exception: "
                << exc.what()
                << std::endl;
    }
    catch (...)
    {
    }
  }
}

void CommonContextCoro::activate_object()
{
  std::lock_guard lock(state_mutex_);
  if (state_ != AS_NOT_ACTIVE)
  {
    Stream::Error stream;
    stream << FNS
           << ": already active";
    throw ActiveObject::AlreadyActive(stream);
  }

  state_ = AS_ACTIVE;
}

void CommonContextCoro::deactivate_object()
{
  {
    std::lock_guard lock(state_mutex_);
    if (state_ == AS_ACTIVE)
    {
      producers_.clear();

      auto* const current_task_processor =
        userver::engine::current_task::GetTaskProcessorOptional();
      for (auto& task: worker_tasks_)
      {
        if (!current_task_processor)
          task.BlockingWait();
        task.Get();
      }

      state_ = AS_DEACTIVATING;
    }
  }

  condition_variable_.notify_all();
}

void CommonContextCoro::wait_object()
{
  std::unique_lock lock(state_mutex_);
  condition_variable_.wait(lock, [this] () {
    return state_ != AS_ACTIVE;
  });

  if (state_ == AS_DEACTIVATING)
  {
    state_ = AS_NOT_ACTIVE;
  }
}

bool CommonContextCoro::active()
{
  std::lock_guard lock(state_mutex_);
  return state_;
}

} // namespace UServerUtils::Grpc::Core::Server