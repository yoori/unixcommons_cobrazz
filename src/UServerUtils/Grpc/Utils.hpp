#ifndef USERVER_GRPC_UTILS_HPP
#define USERVER_GRPC_UTILS_HPP

// STD
#include <optional>
#include <utility>

// THIS
#include <engine/task/task_processor.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/task/task_with_result.hpp>

namespace UServerUtils::Grpc::Utils
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
  auto* current_task_processor =
    engine::current_task::GetTaskProcessorOptional();
  if (current_task_processor)
  {
    if (current_task_processor == &task_processor)
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

} // namespace UServerUtils::Grpc::Utils

#endif //USERVER_GRPC_UTILS_HPP
