#pragma once

#include <vector>
#include <set>

#include <eh/Exception.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/Statistics.hpp>

namespace Generics
{
  class Application
  {
  public:

    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(InvalidOperationOrder, Exception);

  public:

    Application() /*throw (eh::Exception)*/;

    void init(int& argc, char** argv)
      /*throw (Exception, eh::Exception)*/;

    using TimeGenerator = Generics::Time (Application::*)() const;
    using Scenarist = void (Application::*)(TimeGenerator);

    void run(Scenarist make_schedule, TimeGenerator tg)
      /*throw (InvalidOperationOrder, Exception, eh::Exception)*/;

    void stop() /*throw (Exception, eh::Exception)*/;

    Generics::Time rand_time() const /*throw (eh::Exception)*/;

    Generics::Time full_random_time() const /*throw (eh::Exception)*/;

    Generics::Time compact_time_series() const /*throw (eh::Exception)*/;

    void set_test_execution_time(int nt) noexcept;

    /**
     * Scheduling strategies
     */
    void ScheduleMaker(TimeGenerator tg)
      /*throw (Planner::Exception, eh::Exception)*/;

    void SchedulePortionMaker(TimeGenerator tg)
      /*throw (Planner::Exception, eh::Exception)*/;

    void ScheduleMakerUCS97(TimeGenerator tg)
      /*throw (Planner::Exception, eh::Exception)*/;

    void set_message_count(unsigned long new_value) noexcept;

  private:

    void consider_deviation(const Generics::Time& tm1,
                            const Generics::Time& tm2,
                            unsigned long* deviation)
      /*throw (eh::Exception)*/;

    void print_results() /*throw (eh::Exception)*/;

  private:

    class Message : public Goal,
      public ReferenceCounting::AtomicImpl
    {
    public:
      Message(Application* app);
    protected:
      Application* app_;
    };

    class StopMessage : public Message
    {
    public:
      StopMessage(Application* app);

      virtual void deliver() /*throw (eh::Exception)*/;
    };
    using StopMessage_var = ReferenceCounting::QualPtr<StopMessage>;

    class TimedMessage : public Message
    {
    public:
      TimedMessage(Application* app, const Generics::Time& tm)
        /*throw (eh::Exception)*/;

      Generics::Time time() const /*throw (eh::Exception)*/;

      void time(const Generics::Time& tm)
        /*throw (eh::Exception)*/;

      Generics::Time scheduling_time() const /*throw (eh::Exception)*/;

      TimedMessage* scheduling_time(const Generics::Time& tm)
        /*throw (eh::Exception)*/;

      virtual void deliver() /*throw (eh::Exception)*/;

    private:
      Generics::Time time_;
      // At this time message was scheduled into Scheduler
      Generics::Time push_time_;
    };
    using TimedMessage_var = ReferenceCounting::QualPtr<TimedMessage>;

    void deliver_message(TimedMessage* timed_message) noexcept;

  private:

    using Mutex_ = Sync::PosixRWLock;
    using Read_Guard_ = Sync::PosixRGuard;
    using Write_Guard_ = Sync::PosixWGuard;

    bool is_test_successfull_(std::string& error_description) const noexcept;

    mutable Mutex_ lock_;

    Planner_var scheduler_;

    unsigned long  deviation_grid_; // usec
    unsigned long  deviation_max_;  // usec
    unsigned long  message_count_;
    unsigned long  max_sceduling_time_; // sec
    unsigned long  min_sceduling_time_; // sec
    Generics::Time execution_time_;

    Generics::Time start_time_;
    Generics::Time stop_time_;
    unsigned long  deviation_stat_size_;
    std::vector<unsigned long> negative_deviation_;
    std::vector<unsigned long> positive_deviation_;

    Time last_deliver;

//    Statistics::Timed_var preschedule_stat_;
//    Statistics::Timed_var schedule_stat_;
    Generics::ActiveObjectCallback_var callback_;
    Statistics::Collection_var statistics_;
    using PMutex_ = Sync::PosixMutex;
    using PGuard_ = Sync::PosixGuard;
    PMutex_        schedule_events_lock_;

    using Schedule = std::multiset<Generics::Time>;
    Schedule        scheduled_events_;
    Generics::Time  max_gap_;
    Generics::Time  max_gap_planed_moment_;
    Generics::Time  max_gap_schedule_moment_;
    Generics::Time  max_gap_moment_;
    Generics::Time  stop_message_time_;
    std::size_t     processed_events_;
  };
}

/////////////////////////////////////////////////////////////////////////////
// Inlines
/////////////////////////////////////////////////////////////////////////////

namespace Generics
{
  //
  // Application::Message class
  //

  inline Application::Message::Message(Application* app)
    : app_(app)
  {
  }

  //
  // Application::StopMessage class
  //

  inline Application::StopMessage::StopMessage(Application* app)
    : Application::Message(app)
  {
  }

  //
  // Application::TimedMessage class
  //

  inline Application::TimedMessage::TimedMessage(Application* app, const Generics::Time& time)
    /*throw (eh::Exception)*/
    : Application::Message(app),
      time_(time)
  {
  }

  inline Generics::Time Application::TimedMessage::scheduling_time() const
    /*throw (eh::Exception)*/
  {
    return push_time_;
  }

  inline
  Application::TimedMessage* Application::TimedMessage::scheduling_time(const Generics::Time& time)
    /*throw (eh::Exception)*/
  {
    push_time_ = time;
    return this;
  }

  inline Generics::Time Application::TimedMessage::time() const
    /*throw (eh::Exception)*/
  {
    return time_;
  }

  inline void Application::TimedMessage::time(const Generics::Time& time)
    /*throw (eh::Exception)*/
  {
    time_ = time;
  }

}
