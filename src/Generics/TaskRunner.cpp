#include <iostream>
#include <Generics/TaskRunner.hpp>

//#define BUILD_WITH_DEBUG_MESSAGES
#include "Trace.hpp"


namespace Generics
{
  //
  // TaskRunner::TaskRunnerJob class
  //

  TaskRunner::TaskRunnerJob::TaskRunnerJob(
    ActiveObjectCallback* callback,
    unsigned number_of_threads,
    unsigned max_pending_tasks,
    unsigned start_threads)
    /*throw (eh::Exception)*/
    : SingleJob(callback),
      NUMBER_OF_THREADS_(number_of_threads),
      MAX_PENDING_TASKS_(max_pending_tasks),
      // push to number_of_unused_threads_ number of threads already configured
      // in thread runner after activation
      number_of_unused_threads_(start_threads),
      waiting_threads_(0),
      adding_thread_(0)
  {}

  TaskRunner::TaskRunnerJob::~TaskRunnerJob() throw ()
  {}

  void
  TaskRunner::TaskRunnerJob::started(unsigned /*threads*/) throw ()
  {
    //number_of_unused_threads_ = threads;
  }

  void
  TaskRunner::TaskRunnerJob::clear() /*throw (eh::Exception)*/
  {
    Sync::PosixGuard guard(tasks_lock_);
    tasks_.clear();
  }

  void
  TaskRunner::TaskRunnerJob::enqueue_task(
    Task* task,
    const Time* timeout,
    ThreadRunner& thread_runner)
    /*throw (InvalidArgument, Overflow, NotActive, eh::Exception)*/
  {
    if (!task)
    {
      Stream::Error ostr;
      ostr << FNS << "task is NULL";
      throw InvalidArgument(ostr);
    }

    bool overflow = false;
    bool new_task_signal = false;
    bool try_add_thread = false;
    Task_var new_task(ReferenceCounting::add_ref(task));

    while(true)
    {
      Sync::ConditionalGuard lock(not_full_, tasks_lock_);

      // Producer
      try_add_thread = tasks_.size() > number_of_unused_threads_;

      if (MAX_PENDING_TASKS_ == 0 || tasks_.size() < MAX_PENDING_TASKS_)
      {
        tasks_.emplace_back(std::move(new_task));
        new_task_signal = (waiting_threads_ > 0);
        break;
      }
      else
      {
        if(!lock.timed_wait(timeout))
        {
          overflow = true;
          break;
        }
      }
    }

    if(overflow)
    {
      // prepare exception outside lock
      Stream::Error ostr;
      ostr << FNS << "TaskRunner overflow";
      throw Overflow(ostr);
    }

    if(new_task_signal)
    {
      // Wake any working thread
      new_task_.signal();
    }

    if(try_add_thread)
    {
      if(++adding_thread_ == 1)
      {
        try
        {
          add_thread_i_(thread_runner);
        }
        catch(...)
        {
          --adding_thread_;
          throw;
        }
      }

      --adding_thread_;
    }
  }

  void
  TaskRunner::TaskRunnerJob::wait_for_queue_exhausting()
    /*throw (eh::Exception)*/
  {
    // used only in test cases and implemented not effective,
    // but without affecting to main functionality
    //

    while(true)
    {
      {
        Sync::PosixGuard guard(tasks_lock_);
        if (tasks_.empty())
        {
          return;
        }
      }
      Generics::Time wait(0, 300000);
      select(0, 0, 0, 0, &wait);
    }
  }

  void
  TaskRunner::TaskRunnerJob::work() throw ()
  {
    bool number_of_unused_threads_increased = true;

    try
    {
      while(true)
      {
        bool not_full_signal = false;
        Task_var run_task;

        {
          Sync::ConditionalGuard guard(new_task_, tasks_lock_);

          number_of_unused_threads_ += number_of_unused_threads_increased ? 0 : 1;
          number_of_unused_threads_increased = false;

          if(is_terminating())
          {
            --number_of_unused_threads_;
            return;
          }

          while(tasks_.empty())
          {
            ++waiting_threads_;

            guard.wait();

            --waiting_threads_;

            if(is_terminating())
            {
              --number_of_unused_threads_;
              return;
            }
          }

          if(!tasks_.empty())
          {
            run_task.swap(tasks_.front());
            tasks_.pop_front();

            if(MAX_PENDING_TASKS_ > 0)
            {
              not_full_signal = true;
            }
          }

          assert(number_of_unused_threads_ > 0);
          --number_of_unused_threads_;
        }

        if(not_full_signal)
        {
          not_full_.signal();
        }

        try
        {
          run_task->execute();
        }
        catch (const eh::Exception& ex)
        {
          callback()->error(String::SubString(ex.what()));
        }
      }
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FNS << "eh::Exception: " << ex.what();
      callback()->critical(ostr.str());
    }
  }

  void
  TaskRunner::TaskRunnerJob::add_thread_i_(ThreadRunner& thread_runner)
    throw ()
  {
    {
      Sync::PosixGuard lock(tasks_lock_);

      if(tasks_.size() <= number_of_unused_threads_)
      {
        return;
      }

      ++number_of_unused_threads_;
    }

    try
    {
      if (!thread_runner.running() ||
        thread_runner.running() == thread_runner.number_of_jobs())
      {
        return;
      }

      thread_runner.start_one();
    }
    catch(const eh::Exception& ex)
    {
      {
        Sync::PosixGuard lock(tasks_lock_);
        --number_of_unused_threads_;
      }

      Stream::Error ostr;
      ostr << FNS << "eh::Exception: " << ex.what();
      callback()->warning(ostr.str());
    }
  }

  void
  TaskRunner::TaskRunnerJob::terminate() throw ()
  {
    Sync::PosixGuard guard(tasks_lock_);
    new_task_.broadcast();
  }

  //
  // TaskRunner class
  //

  TaskRunner::TaskRunner(ActiveObjectCallback* callback,
    unsigned threads_number, size_t stack_size,
    unsigned max_pending_tasks, unsigned start_threads)
    /*throw (InvalidArgument, Exception, eh::Exception)*/
    : ActiveObjectCommonImpl(
        TaskRunnerJob_var(
          new TaskRunnerJob(
            callback,
            threads_number,
            max_pending_tasks,
            // we need to start one thread minimum because task can be enqueued before activation
            std::max(start_threads, 1u)
            )),
        threads_number,
        stack_size,
        std::max(start_threads, 1u)
        ),
      job_(static_cast<TaskRunnerJob&>(*SINGLE_JOB_))
  {}

  void
  TaskRunner::activate_object()
  {
    ActiveObjectCommonImpl::activate_object();

    // TODO: start deferred threads
  }

  TaskRunner::~TaskRunner() throw ()
  {}
}
