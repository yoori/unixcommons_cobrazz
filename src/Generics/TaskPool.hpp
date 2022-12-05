#ifndef GENERICS_TASKPOOL_HPP
#define GENERICS_TASKPOOL_HPP

#include <vector>

#include <ReferenceCounting/Deque.hpp>

#include <Generics/AtomicInt.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/TaskRunner.hpp>
#include <Generics/CompositeActiveObject.hpp>

namespace Generics
{
  /**
   * Performs tasks in several threads simultaneously.
   */
  class TaskPool :
    public TaskExecutor,
    public CompositeActiveObject,
    public ReferenceCounting::AtomicImpl
  {
  public:
    typedef TaskExecutor::Exception Exception;
    typedef TaskExecutor::Overflow Overflow;
    typedef TaskExecutor::NotActive NotActive;
    typedef ActiveObject::InvalidArgument InvalidArgument;

    /**
     * Constructor
     * @param callback not null callback is called on errors
     * @param threads_number number of working threads
     * @param stack_size their stack sizes
     */
    TaskPool(ActiveObjectCallback* callback,
      unsigned threads_number,
      size_t stack_size = 0)
      /*throw (InvalidArgument, Exception, eh::Exception)*/;

    /**
     * Enqueues a task
     * @param task task to enqueue. Number of references is not increased
     * @param timeout maximal absolute wait time before fail on mutex lock
     * until the task is put in the queue. NULL timeout means no wait.
     * If you put limitations on the size of the queue, and it's full,
     * method waits for the release up to timeout
     */
    void
    enqueue_task(Task* task, const Time* timeout = 0)
      /*throw (InvalidArgument, Overflow, NotActive, eh::Exception)*/;

    virtual
    void
    deactivate_object() throw();

    /**
     * Returns number of tasks recently being enqueued
     * This number does not have much meaning in MT environment
     * @return number of tasks enqueued
     */
    unsigned
    task_count() const throw ();

    /**
     * Clear task queue
     */
    virtual
    void
    clear() /*throw (eh::Exception)*/;

  protected:
    virtual
    ~TaskPool() throw ();

  private:
    class TaskQueueProcessor;

    struct TaskQueue : public ReferenceCounting::AtomicImpl
    {
      friend class TaskQueueProcessor;

    public:
      TaskQueue() throw();

      void
      enqueue_task(Task* task, const Time* timeout)
        /*throw (InvalidArgument, Overflow, NotActive, eh::Exception)*/;

      void
      terminate() throw ();

      virtual
      void
      clear() /*throw (eh::Exception)*/;

    protected:
      virtual
      ~TaskQueue() throw()
      {}

    protected:
      typedef ReferenceCounting::Deque<Task_var> Tasks;

    protected:
      mutable Sync::PosixMutex tasks_lock_;
      Tasks tasks_;
      Sync::Conditional new_task_;
      unsigned waiting_threads_;
    };

    typedef ReferenceCounting::QualPtr<TaskQueue> TaskQueue_var;

    class TaskQueueProcessor: public ActiveObjectCommonImpl
    {
    public:
      class Job : public ActiveObjectCommonImpl::SingleJob
      {
      public:
        Job(
          ActiveObjectCallback* callback,
          TaskQueue* task_queue)
          /*throw (eh::Exception)*/;

        virtual
        void
        work() throw ();

        virtual
        void
        terminate() throw ();

      protected:
        virtual
        ~Job() throw ();

      protected:
        TaskQueue_var task_queue_;
      };

    public:
      TaskQueueProcessor(
        ActiveObjectCallback* callback,
        TaskQueue* task_queue,
        unsigned stack_size)
        throw();

    protected:
      typedef ReferenceCounting::QualPtr<Job> Job_var;
    };

    typedef std::vector<TaskQueue_var> TaskQueueArray;

  private:
    TaskQueueArray task_queues_;
    AtomicUInt task_queue_pos_;

    /*
    Sync::PosixSpinLock task_queue_pos_lock_;
    unsigned task_queue_pos_;
    */
  };

  typedef ReferenceCounting::QualPtr<TaskPool> TaskPool_var;
  typedef ReferenceCounting::FixedPtr<TaskPool> FixedTaskPool_var;
}

#endif
