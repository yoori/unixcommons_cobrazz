// BOOST
#include <boost/range/adaptors.hpp>

// STD
#include <sstream>

// USERVER
#include <userver/hostinfo/cpu_limit.hpp>

// USERVER PRIVATE HEADERS
#include <engine/task/task_counter.hpp>
#include <engine/task/task_processor.hpp>

// THIS
#include <UServerUtils/TaskProcessorContainer.hpp>

namespace UServerUtils
{

namespace Aspect
{

const char TASK_PROCESSOR_CONTAINER[] = "TASK_PROCESSOR_CONTAINER";

} // namespace Aspect

TaskProcessorContainer::TaskProcessorContainer(
  Logger* logger,
  const CoroPoolConfig& coro_pool_config,
  const EventThreadPoolConfig& event_thread_pool_config,
  const TaskProcessorConfig& main_task_processor_config)
  : logger_(ReferenceCounting::add_ref(logger))
{
  if (!logger_)
  {
    Stream::Error stream;
    stream << FNS
           << ": logger is null";
    throw Exception(stream);
  }

  userver::engine::coro::PoolConfig pool_config;
  pool_config.initial_size = coro_pool_config.initial_size;
  pool_config.max_size = coro_pool_config.max_size;
  pool_config.stack_size = pool_config.stack_size;

  userver::engine::ev::ThreadPoolConfig thread_pool_config;
  thread_pool_config.threads =
    event_thread_pool_config.threads;
  thread_pool_config.thread_name =
    event_thread_pool_config.thread_name;
  thread_pool_config.ev_default_loop_disabled =
    event_thread_pool_config.ev_default_loop_disabled;

  task_processor_pools_ = std::make_shared<TaskProcessorPools>(
    pool_config,
    thread_pool_config);

  add_task_processor_helper(main_task_processor_config, true);
}

TaskProcessorContainer::~TaskProcessorContainer()
{
  using TaskCounter = userver::engine::impl::TaskCounter;

  try
  {
    for (auto& [name, task_processor] : name_to_task_processor_)
    {
      task_processor->InitiateShutdown();
    }

    const auto indicators = name_to_task_processor_ | boost::adaptors::map_values |
      boost::adaptors::transformed(
        [](const auto& task_processor_ptr) -> const auto& {
          const auto& task_processor = *task_processor_ptr;
          return task_processor.GetTaskCounter();
        });

    while (TaskCounter::AnyMayHaveTasksAlive(indicators))
    {
      std::this_thread::sleep_for(std::chrono::milliseconds{10});
    }

    name_to_task_processor_.clear();
    task_processor_pools_.reset();
  }
  catch (...)
  {
  }
}

void TaskProcessorContainer::add_task_processor(
  const TaskProcessorConfig& config)
{
  add_task_processor_helper(config, false);
}

TaskProcessor& TaskProcessorContainer::get_task_processor(
  const std::string& name)
{
  auto it = name_to_task_processor_.find(name);
  if (it == name_to_task_processor_.end())
  {
    Stream::Error stream;
    stream << FNS
           << ": not existing task processor with name="
           << name;
    throw Exception(stream);
  }

  return *it->second;
}

TaskProcessor& TaskProcessorContainer::get_main_task_processor()
{
  return get_task_processor(main_task_processor_name_);
}

void TaskProcessorContainer::add_task_processor_helper(
  const TaskProcessorConfig& config,
  const bool is_main)
{
  if (config.name.empty())
  {
    Stream::Error stream;
    stream << FNS
           << ": name of task processor can't be empty";
    throw Exception(stream);
  }

  userver::engine::TaskProcessorConfig processor_config;
  processor_config.name = config.name;
  processor_config.should_guess_cpu_limit = config.should_guess_cpu_limit;
  processor_config.worker_threads = config.worker_threads;
  processor_config.thread_name = config.thread_name;

  switch (config.os_scheduling)
  {
    case TaskProcessorConfig::OsScheduling::Normal:
      processor_config.os_scheduling = userver::engine::OsScheduling::kNormal;
      break;
    case TaskProcessorConfig::OsScheduling::LowPriority:
      processor_config.os_scheduling = userver::engine::OsScheduling::kLowPriority;
      break;
    case TaskProcessorConfig::OsScheduling::Idle:
      processor_config.os_scheduling = userver::engine::OsScheduling::kIdle;
      break;
  }

  processor_config.task_trace_every = 1000;
  // task_trace_max_csw -> 0 ---> disable trace
  processor_config.task_trace_max_csw = 0;

  if (name_to_task_processor_.count(processor_config.name) == 1)
  {
    Stream::Error stream;
    stream << FNS
           << ": task processor with name="
           << processor_config.name
           << " already exist";
    throw Exception(stream);
  }

  if (is_main)
  {
    main_task_processor_name_ = processor_config.name;
    if (processor_config.should_guess_cpu_limit)
    {
      const auto guess_cpu =
        guess_cpu_limit(processor_config.name);
      if (guess_cpu)
      {
        processor_config.worker_threads = *guess_cpu;
      }
    }
  }

  auto result = name_to_task_processor_.try_emplace(
    processor_config.name,
    std::make_unique<TaskProcessor>(
      processor_config,
      task_processor_pools_));

  auto& task_processor = *result.first->second;

  TaskProcessorSettings settings;
  settings.sensor_wait_queue_time_limit = config.sensor_wait_queue_time_limit;
  settings.wait_queue_time_limit = config.wait_queue_time_limit;
  settings.wait_queue_length_limit = config.wait_queue_length_limit;
  settings.profiler_force_stacktrace = false;
  settings.profiler_execution_slice_threshold = std::chrono::microseconds{0};

  switch (config.overload_action)
  {
  case TaskProcessorConfig::OverloadAction::Ignore:
    settings.overload_action = TaskProcessorSettings::OverloadAction::kIgnore;
    break;
  case TaskProcessorConfig::OverloadAction::Cancel:
    settings.overload_action = TaskProcessorSettings::OverloadAction::kCancel;
    break;
  }
  task_processor.SetSettings(settings);
}

std::optional<std::size_t>
TaskProcessorContainer::guess_cpu_limit(
  const std::string& name_task_processor)
{
  const auto cpu_limit = userver::hostinfo::CpuLimit();
  if (!cpu_limit)
  {
    return {};
  }

  const auto hw_concurrency = std::thread::hardware_concurrency();
  const auto hw_threads_estimate =
    hw_concurrency ? hw_concurrency : k_default_threads_estimate;

  auto cpu = std::lround(*cpu_limit);
  if (cpu > 0 && static_cast<unsigned int>(cpu) < hw_threads_estimate * 2)
  {
    if (cpu < 3)
    {
      cpu = 3;
    }

    std::ostringstream stream;
    stream << FNS
           << "Using CPU limit from env CPU_LIMIT ("
           << cpu
           << ") for worker_threads of task processor '"
           << name_task_processor
           << "', ignoring config value ";
    logger_->info(stream.str(), Aspect::TASK_PROCESSOR_CONTAINER);

    return cpu;
  }

  std::ostringstream stream;
  stream << FNS
         << "CPU limit from env CPU_LIMIT ("
         << *cpu_limit
         << ") looks very different from the estimated number of "
         << "hardware threads ("
         << hw_threads_estimate
         << "), worker_threads from the static config will be used";
  logger_->warning(stream.str(), Aspect::TASK_PROCESSOR_CONTAINER);

  return {};
}

} // namespace UServerUtils
