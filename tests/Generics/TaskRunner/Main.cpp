#include <unistd.h>

#include <iostream>
#include <fstream>

#include <Generics/TaskRunner.hpp>


class TaskImpl :
  public Generics::Task,
#if 1
  // Usually fail
  public ReferenceCounting::DefaultImpl<>
#else
  // Must not fail
  public ReferenceCounting::AtomicImpl
#endif
{
public:
  TaskImpl(Generics::TaskRunner* task_runner)
    throw ();

  virtual void
  execute() throw ();

private:
  Generics::TaskRunner_var task_runner_;
};

TaskImpl::TaskImpl(Generics::TaskRunner* task_runner)
  throw ()
  : task_runner_(ReferenceCounting::add_ref(task_runner))
{
}

void
TaskImpl::execute() throw ()
{
  task_runner_->enqueue_task(this);
}

int
main()
{
  try
  {
    Generics::TaskRunner_var task_runner(new Generics::TaskRunner(0, 4));
    for (int i = 0; i < 3; i++)
    {
      task_runner->enqueue_task(Generics::Task_var(new TaskImpl(task_runner)));
    }

    task_runner->activate_object();
    sleep(10);
    task_runner->deactivate_object();
    task_runner->wait_object();
    task_runner->clear();
    return 0;
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << ex.what() << std::endl;
  }
  return -1;
}
