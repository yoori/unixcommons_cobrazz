#pragma once

#include <ReferenceCounting/List.hpp>

#include <Generics/ActiveObject.hpp>


namespace Generics
{
  class Goal : public virtual ReferenceCounting::Interface
  {
  public:
    /**
     * Callback function to be called from the scheduler
     */
    virtual void deliver() /*throw (eh::Exception)*/ = 0;
  };
  using Goal_var = ReferenceCounting::QualPtr<Goal>;

  class Planner : public ActiveObjectCommonImpl
  {
  public:
    DECLARE_EXCEPTION(Exception, ActiveObject::Exception);

    /**
     * Constructor
     * @param callback Reference countable callback object to be called
     * for errors
     * @param stack_size stack size for working thread
     * @param delivery_time_adjustment Should delivery_time_shift_ be used
     * for messages' time shift
     */
    Planner(ActiveObjectCallback* callback,
      size_t stack_size = 0, bool delivery_time_adjustment = false)
      /*throw (InvalidArgument, eh::Exception)*/;

    /**
     * Adds goal to the queue. Goal's reference counter is incremented.
     * On error it is unchanged (and object will be freed in the caller).
     * @param goal Object to enqueue
     * @param time Timestamp to match
     */
    void schedule(Goal* goal, const Time& time)
      /*throw (InvalidArgument, Exception, eh::Exception)*/;

    /**
     * Tries to remove goal from the queue.
     * @param goal Object to remove
     * @return number of entries removed
     */
    unsigned unschedule(const Goal* goal)
      /*throw (eh::Exception)*/;

    /**
     * Clearance of messages' queue
     */
    virtual void clear() /*throw (eh::Exception)*/;

  protected:
    /**
     * Destructor
     * Decreases all unmatched messages' reference counters
     */
    virtual ~Planner() noexcept;

  private:
    class PlannerJob : public SingleJob
    {
    public:
      PlannerJob(ActiveObjectCallback* callback,
        bool delivery_time_adjustment) /*throw (eh::Exception)*/;

      virtual void work() noexcept;

      virtual void terminate() noexcept;

      void schedule(Goal* goal, const Time& time)
        /*throw (InvalidArgument, Exception, eh::Exception)*/;

      unsigned unschedule(const Goal* goal)
        /*throw (eh::Exception)*/;

      void clear() noexcept;

    protected:
      virtual ~PlannerJob() noexcept;

      /**
       * Element of messages' queue. Composition of Message and
       * associated Time.
       */
      class TimedMessage
      {
      public:
        TimedMessage() noexcept;

        TimedMessage(TimedMessage&) = default;

        /**
         * Constructor
         * @param time Associated time
         * @param goal Shared ownership on goal
         */
        TimedMessage(const Time& time, Goal* goal) noexcept;

        /**
         * Holding time
         * @return Associated time
         */
        const Time& time() const noexcept;

        /**
         * Calls deliver() on owned goal
         */
        void deliver() /*throw (eh::Exception)*/;

        /**
         * Checks if it holds the goal
         * @param goal goal to check against
         * @return true if they coincide
         */
        bool is_goal(const Goal* goal) const noexcept;

      private:
        Time time_;
        Goal_var goal_;
      };
      using TimedList = ReferenceCounting::List<TimedMessage>;

      mutable Sync::Conditional new_event_in_schedule_;
      bool have_new_events_;  // Predicate for condition!

      TimedList messages_;
      bool delivery_time_adjustment_;
      Time delivery_time_shift_;
    };
    using PlannerJob_var = ReferenceCounting::FixedPtr<PlannerJob>;

    PlannerJob& job_;
  };
  using Planner_var = ReferenceCounting::QualPtr<Planner>;
  using FixedPlanner_var = ReferenceCounting::FixedPtr<Planner>;
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace Generics
{
  //
  // Planner::TimedMessage class
  //

  inline Planner::PlannerJob::TimedMessage::TimedMessage() noexcept
  {
  }

  inline Planner::PlannerJob::TimedMessage::TimedMessage( const Time& time, Goal* goal) noexcept
    : time_(time), goal_(ReferenceCounting::add_ref(goal))
  {
  }

  inline const Time& Planner::PlannerJob::TimedMessage::time() const noexcept
  {
    return time_;
  }

  inline void Planner::PlannerJob::TimedMessage::deliver() /*throw (eh::Exception)*/
  {
    goal_->deliver();
  }

  inline bool Planner::PlannerJob::TimedMessage::is_goal(const Goal* goal) const noexcept
  {
    return goal == goal_;
  }


  //
  // Planner class
  //

  inline void Planner::schedule(Goal* goal, const Time& time)
    /*throw (InvalidArgument, Exception, eh::Exception)*/
  {
    job_.schedule(goal, time);
  }

  inline unsigned Planner::unschedule(const Goal* goal)
    /*throw (eh::Exception)*/
  {
    return job_.unschedule(goal);
  }

  inline void Planner::clear() /*throw (eh::Exception)*/
  {
    job_.clear();
  }
}
