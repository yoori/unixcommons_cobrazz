#ifndef USERVER_TASKPROCESSORCONTAINER_HPP
#define USERVER_TASKPROCESSORCONTAINER_HPP

// STD
#include <memory>
#include <string>
#include <optional>
#include <unordered_map>

// USERVER
#include <engine/coro/pool_config.hpp>
#include <engine/task/task_processor_config.hpp>
#include <engine/task/task_processor_pools.hpp>
#include <userver/engine/task/task_processor.hpp>

// THIS
#include <eh/Exception.hpp>
#include <Generics/Uncopyable.hpp>
#include <Logger/Logger.hpp>
#include <UServerUtils/Config.hpp>

namespace UServerUtils
{

using TaskProcessor = userver::engine::TaskProcessor;
using TaskProcessorPtr = std::unique_ptr<TaskProcessor>;

class TaskProcessorContainer final
  : private Generics::Uncopyable
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

private:
  using TaskProcessorSettings = userver::engine::TaskProcessorSettings;
  using TaskProcessorPools = userver::engine::impl::TaskProcessorPools;
  using TaskProcessorPoolsPtr = std::shared_ptr<TaskProcessorPools>;
  using TaskProcessorName = std::string;
  using NameToTaskProcessor = std::unordered_map<TaskProcessorName, TaskProcessorPtr>;

public:
  ~TaskProcessorContainer();

  TaskProcessor& get_task_processor(const std::string& name);

  TaskProcessor& get_main_task_processor();

private:
  explicit TaskProcessorContainer(
    Logger* logger,
    const CoroPoolConfig& coro_pool_config,
    const EventThreadPoolConfig& event_thread_pool_config,
    const TaskProcessorConfig& main_task_processor_config);

  void add_task_processor(
    const TaskProcessorConfig& config);

  void add_task_processor_helper(
    const TaskProcessorConfig& config,
    const bool is_main = false);

  std::optional<std::size_t> guess_cpu_limit(
    const std::string& name_task_processor);

private:
  friend class TaskProcessorContainerBuilder;

  constexpr static std::size_t k_default_threads_estimate = 128;

  Logger_var logger_;

  TaskProcessorPoolsPtr task_processor_pools_;

  std::string main_task_processor_name_;

  NameToTaskProcessor name_to_task_processor_;
};

using TaskProcessorContainerPtr = std::unique_ptr<TaskProcessorContainer>;

} // namespace UServerUtils

#endif //USERVER_TASKPROCESSORCONTAINER_HPP
