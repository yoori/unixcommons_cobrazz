#ifndef USERVER_GRPC_TASKPROCESSORCONTAINERBUILDER_HPP
#define USERVER_GRPC_TASKPROCESSORCONTAINERBUILDER_HPP

// STD
#include <memory>

// THIS
#include <eh/Exception.hpp>
#include <Generics/Uncopyable.hpp>
#include "TaskProcessorContainer.hpp"

namespace UServerUtils
{
namespace Grpc
{

class TaskProcessorContainerBuilder final
  : private Generics::Uncopyable
{
public:
  using Logger_var = Logging::Logger_var;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  explicit TaskProcessorContainerBuilder(
    const Logger_var& logger,
    const CoroPoolConfig& coro_pool_config,
    const EventThreadPoolConfig& event_thread_pool_config,
    const TaskProcessorConfig& main_task_processor_config);

  ~TaskProcessorContainerBuilder() = default;

  void add_task_processor(
    const TaskProcessorConfig& config);

private:
  TaskProcessorContainerPtr build();

private:
  friend class Manager;

  TaskProcessorContainerPtr task_processor_container_;
};

using TaskProcessorContainerBuilderPtr =
  std::unique_ptr<TaskProcessorContainerBuilder>;

} // namespace Grpc
} // namespace UServerUtils

#endif //USERVER_GRPC_TASKPROCESSORCONTAINERBUILDER_HPP
