#ifndef SCHEDULER_TEST_NUMBER_TWO_INCLUDED
#define SCHEDULER_TEST_NUMBER_TWO_INCLUDED

#include <eh/Exception.hpp>
#include <Generics/Scheduler.hpp>

namespace Generics
{

  class Application
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  public:

    Application() /*throw(eh::Exception)*/;

    void run() /*throw (Exception, eh::Exception)*/;

  private:

    Generics::ActiveObjectCallback_var callback_;
    Planner_var scheduler1_;
    Planner_var scheduler2_;
  };
}

#endif // SCHEDULER_TEST_NUMBER_TWO_INCLUDED
