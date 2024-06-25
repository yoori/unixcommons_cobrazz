#ifndef USERVER_UTILS_HPP
#define USERVER_UTILS_HPP

// STD
#include <optional>
#include <cstdio>
#include <utility>

// USERVER
#include <userver/engine/task/task.hpp>
#include <userver/engine/task/task_processor.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/task/task_with_result.hpp>

// THIS
#include <utils/signal_catcher.hpp>

namespace UServerUtils::Utils
{

namespace engine = userver::engine;
using TaskProcessor = engine::TaskProcessor;
using Importance = engine::Task::Importance;
using Deadline = engine::Deadline;

template<typename Func, typename ...Args>
inline auto run_in_coro(
  TaskProcessor& task_processor,
  Importance importance,
  Deadline deadline,
  Func&& func,
  Args&& ...args)
{
  const bool is_task_processor_thread =
    userver::engine::current_task::IsTaskProcessorThread();
  if (is_task_processor_thread)
  {
    auto& current_task_processor =
      engine::current_task::GetTaskProcessor();
    if (&current_task_processor == &task_processor)
    {
      return std::forward<Func>(func)(std::forward<Args>(args)...);
    }
    else
    {
      return engine::impl::MakeTaskWithResult<engine::TaskWithResult>(
        task_processor,
        importance,
        deadline,
        std::forward<Func>(func),
        std::forward<Args>(args)...).Get();
    }
  }

  auto task =
    engine::impl::MakeTaskWithResult<engine::TaskWithResult>(
      task_processor,
      importance,
      deadline,
      std::forward<Func>(func),
      std::forward<Args>(args)...);
  task.BlockingWait();
  return task.Get();
}

class SignalCatcher final
{
public:
  SignalCatcher(std::initializer_list<int> signals)
    : signal_catcher_(signals)
  {
  }

  ~SignalCatcher() noexcept(false) = default;

  int catch_signal()
  {
    return signal_catcher_.Catch();
  }

private:
  userver::utils::SignalCatcher signal_catcher_;
};

} // namespace UServerUtils::Utils

#endif //USERVER_UTILS_HPP