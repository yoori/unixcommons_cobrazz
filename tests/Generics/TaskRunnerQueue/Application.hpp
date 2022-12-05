// Application.hpp
#ifndef _TEST_APPLICATION_TASK_RUNNER_HPP_INCLUDED_
#define _TEST_APPLICATION_TASK_RUNNER_HPP_INCLUDED_ 

#include <Generics/TaskRunner.hpp>
#include <TestCommons/ActiveObjectCallback.hpp>

class TestTasker
{
public:
  TestTasker() /*throw (eh::Exception)*/;

  virtual
  ~TestTasker() throw ();

  void
  do_test() /*throw (eh::Exception)*/;

  void
  do_release_queue_test() /*throw (eh::Exception)*/;

private:
  void
  spawn_tasker_(std::size_t threads_number, std::size_t queue_size = 0)
    /*throw (eh::Exception)*/;

  Generics::ActiveObjectCallback_var task_runner_callback_;
  Generics::TaskRunner_var task_runner_;
};

//////////////////////////////////////////////////////////////////////////
// Implementations

TestTasker::TestTasker() /*throw (eh::Exception)*/
  : task_runner_callback_(
      new TestCommons::ActiveObjectCallbackStreamImpl(
        std::cerr, "TaskRunnerQueue"))
{
}

TestTasker::~TestTasker() throw ()
{
  if (task_runner_.in())
  {
    task_runner_->deactivate_object();
    task_runner_->wait_object();
  }
}

void
TestTasker::spawn_tasker_(std::size_t threads_number,
                          std::size_t queue_size)
  /*throw (eh::Exception)*/
{
  if (task_runner_.in())
  {
    task_runner_->deactivate_object();
    task_runner_->wait_object();
  }
  task_runner_ =
    new Generics::TaskRunner(task_runner_callback_,
      threads_number, 0, queue_size);
  task_runner_->activate_object();
}

#endif  // _TEST_APPLICATION_TASK_RUNNER_HPP_INCLUDED_
