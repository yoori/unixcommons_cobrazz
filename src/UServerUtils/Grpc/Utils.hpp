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
#include <utils/signal_catcher.hpp>

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

namespace Internal
{

template<class T>
class Function;

template<class R, class ...Args>
class Function<R(Args...)>
{
public:
  using Func = std::function<R(Args...)>;

public:
  template<class F,
           class = std::enable_if_t<
             std::is_invocable_v<std::decay_t<F>, Args...>>>
  Function(F&& f)
  {
    auto ptr = std::make_shared<std::decay_t<F>>(std::forward<F>(f));
    func_ = [ptr = std::move(ptr)] (Args... args) {
      return (*ptr)(std::forward<Args>(args)...);
    };
  }

  Function(const Function&) = default;
  Function(Function&&) = default;
  Function& operator=(const Function&) = default;
  Function& operator=(Function&&) = default;

  R operator()(Args... args)
  {
    return func_(std::forward<Args>(args)...);
  }

private:
  Func func_;
};

} // namespace Internal

} // namespace UServerUtils::Grpc::Utils

#endif //USERVER_GRPC_UTILS_HPP