#include <iostream>
#include <Generics/TaskPool.hpp>

//#define BUILD_WITH_DEBUG_MESSAGES
#include "Trace.hpp"

namespace Generics
{
  //
  // class TaskPool::TaskQueue
  //

  TaskPool::TaskQueue::TaskQueue() throw()
    : waiting_threads_(0)
  {}

  void
  TaskPool::TaskQueue::enqueue_task(
    Task* task,
    const Time* /*timeout*/)
    /*throw (InvalidArgument, Overflow, NotActive, eh::Exception)*/
  {
    if (!task)
    {
      Stream::Error ostr;
      ostr << FNS << "task is NULL";
      throw InvalidArgument(ostr);
    }

    bool new_task_signal = false;
    Task_var new_task(ReferenceCounting::add_ref(task));

    {
      Sync::PosixGuard lock(tasks_lock_);

      tasks_.emplace_back(std::move(new_task));
      new_task_signal = (waiting_threads_ > 0);
    }

    if(new_task_signal)
    {
      // Wake any working thread
      new_task_.signal();
    }
  }

  void
  TaskPool::TaskQueue::terminate() throw ()
  {
    Sync::PosixGuard guard(tasks_lock_);
    new_task_.broadcast();
  }

  void
  TaskPool::TaskQueue::clear() /*throw (eh::Exception)*/
  {
    Tasks destroy_tasks;

    Sync::PosixGuard guard(tasks_lock_);
    destroy_tasks.swap(tasks_);
  }

  //
  // class TaskPool::TaskQueueProcessor
  //
  TaskPool::TaskQueueProcessor::TaskQueueProcessor(
    ActiveObjectCallback* callback,
    TaskQueue* task_queue,
    unsigned stack_size)
    throw()
    : ActiveObjectCommonImpl(
        Job_var(
          new Job(callback, task_queue)),
        1,
        stack_size,
        1)
  {}

  //
  // TaskPool::Job class
  //

  TaskPool::TaskQueueProcessor::Job::Job(
    ActiveObjectCallback* callback,
    TaskQueue* task_queue)
    /*throw (eh::Exception)*/
    : SingleJob(callback),
      task_queue_(ReferenceCounting::add_ref(task_queue))
  {}

  void
  TaskPool::TaskQueueProcessor::Job::terminate() throw ()
  {
    task_queue_->terminate();
  }

  TaskPool::TaskQueueProcessor::Job::~Job() throw ()
  {}

  void
  TaskPool::TaskQueueProcessor::Job::work() throw ()
  {
    TaskQueue* task_queue = task_queue_.in();

    try
    {
      while(true)
      {
        Task_var run_task;

        {
          Sync::ConditionalGuard guard(task_queue->new_task_, task_queue->tasks_lock_);

          if(is_terminating())
          {
            return;
          }

          while(task_queue->tasks_.empty())
          {
            ++task_queue->waiting_threads_;

            guard.wait();

            --task_queue->waiting_threads_;

            if(is_terminating())
            {
              return;
            }
          }

          if(!task_queue->tasks_.empty())
          {
            run_task.swap(task_queue->tasks_.front());
            task_queue->tasks_.pop_front();
          }
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

  //
  // TaskPool class
  //

  TaskPool::TaskPool(
    ActiveObjectCallback* callback,
    unsigned threads_number,
    size_t stack_size)
    /*throw (InvalidArgument, Exception, eh::Exception)*/
    : task_queue_pos_(0)
  {
    for(unsigned job_i = 0; job_i < threads_number; ++job_i)
    {
      task_queues_.push_back(TaskQueue_var());
    }

    for(unsigned job_i = 0; job_i < threads_number; ++job_i)
    {
      task_queues_[job_i] = new TaskQueue();

      add_child_object(
        ActiveObject_var(
          new TaskQueueProcessor(
            callback,
            task_queues_[job_i],
            stack_size)));
    }
  }

  void
  TaskPool::enqueue_task(Task* task, const Time* timeout)
    /*throw (InvalidArgument, Overflow, NotActive, eh::Exception)*/
  {
    unsigned pos = ++task_queue_pos_;
    task_queues_[pos % task_queues_.size()]->enqueue_task(task, timeout);
  }

  void
  TaskPool::deactivate_object() throw()
  {
    CompositeActiveObject::deactivate_object();

    for(auto it = task_queues_.begin(); it != task_queues_.end(); ++it)
    {
      (*it)->terminate();
    }
  }

  void
  TaskPool::clear() /*throw (eh::Exception)*/
  {
    for(auto it = task_queues_.begin(); it != task_queues_.end(); ++it)
    {
      (*it)->clear();
    }
  }

  TaskPool::~TaskPool() throw ()
  {}
}
