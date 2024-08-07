// THIS
#include <UServerUtils/Grpc/Server/CommonContextCoro.hpp>

namespace UServerUtils::Grpc::Server
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
{
}

CommonContextCoro::~CommonContextCoro()
{
  try
  {
    deactivate_object();
  }
  catch (const eh::Exception& exc)
  {
    std::cerr << FNS << "Exception: " << exc.what();
  }
  catch (...)
  {
    std::cerr << FNS << "Exception: Unknown error";
  }
}

void CommonContextCoro::activate_object_()
{
  UServerUtils::Component::CompositeActiveObjectBase::SimpleActiveObject::wait_object_();
}

void CommonContextCoro::deactivate_object_()
{
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

void CommonContextCoro::wait_object_()
{
  UServerUtils::Component::CompositeActiveObjectBase::SimpleActiveObject::wait_object_();
}

} // namespace UServerUtils::Grpc::Server