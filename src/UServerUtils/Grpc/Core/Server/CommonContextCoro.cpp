// THIS
#include <UServerUtils/Grpc/Core/Server/CommonContextCoro.hpp>

namespace UServerUtils::Grpc::Core::Server
{
  namespace Aspect
  {
    const char* COMMON_CONTEXT_CORO = "COMMON_CONTEXT_CORO";
  } // namespace Aspect

  CommonContextCoro::CommonContextCoro(
    Logger* logger,
    const MaxSizeQueue max_size_queue)
    : logger_(ReferenceCounting::add_ref(logger)),
      max_size_queue_(max_size_queue)
  {}

  CommonContextCoro::~CommonContextCoro()
  {}

  void CommonContextCoro::deactivate_object_()
  {
    std::unique_lock<std::mutex> lock(lock_);

    producers_.clear();

    const bool is_task_processor_thread =
      userver::engine::current_task::IsTaskProcessorThread();
    for (auto& task: worker_tasks_)
    {
      if (!is_task_processor_thread)
      {
	task.BlockingWait();
      }
      task.Get();
    }
  }
} // namespace UServerUtils::Grpc::Core::Server
