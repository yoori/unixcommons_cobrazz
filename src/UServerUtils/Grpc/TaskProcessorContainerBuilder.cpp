// THIS
#include <UServerUtils/Grpc/TaskProcessorContainerBuilder.hpp>

namespace UServerUtils::Grpc
{

TaskProcessorContainerBuilder::TaskProcessorContainerBuilder(
  Logger* logger,
  const CoroPoolConfig& coro_pool_config,
  const EventThreadPoolConfig& event_thread_pool_config,
  const TaskProcessorConfig& main_task_processor_config)
  : task_processor_container_(
      new TaskProcessorContainer(
        logger,
        coro_pool_config,
        event_thread_pool_config,
        main_task_processor_config))
{
}

void TaskProcessorContainerBuilder::add_task_processor(
  const TaskProcessorConfig& config)
{
  if (!task_processor_container_)
  {
    Stream::Error stream;
    stream << FNS
           << ": TaskProcessorCintainer already built";
    throw Exception(stream);
  }

  task_processor_container_->add_task_processor(config);
}

TaskProcessorContainerPtr
TaskProcessorContainerBuilder::build()
{
  return std::move(task_processor_container_);
}

} // namespace UServerUtils::Grpc