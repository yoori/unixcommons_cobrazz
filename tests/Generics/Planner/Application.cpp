// @file Application.cpp

#include <Logger/StreamLogger.hpp>
#include <Logger/ActiveObjectCallback.hpp>

#include <Generics/Scheduler.hpp>
#include <Generics/CompositeActiveObject.hpp>

class CompositeActiveObjectImpl :
  public Generics::CompositeActiveObject,
  public virtual ReferenceCounting::AtomicImpl
{
};

namespace
{
  Logging::FLogger_var logger(new Logging::OStream::Logger(
    Logging::OStream::Config(std::cerr)));
  Generics::ActiveObjectCallback_var callback(
    new Logging::ActiveObjectCallbackImpl(logger));
}

class ActivateDeactivatePlanner
{
public:
  ActivateDeactivatePlanner() /*throw (eh::Exception)*/;
  void
  test() /*throw (eh::Exception)*/;

private:
  Generics::CompositeActiveObject_var active_objects_composite_;
};

ActivateDeactivatePlanner::ActivateDeactivatePlanner() /*throw (eh::Exception)*/
  : active_objects_composite_(new CompositeActiveObjectImpl)
{
  Generics::Planner_var scheduler(
    new Generics::Planner(callback));

  active_objects_composite_->add_child_object(scheduler);
}

void
ActivateDeactivatePlanner::test() /*throw (eh::Exception)*/
{
  for (std::size_t i = 0; i < 10000; ++i)
  {
    active_objects_composite_->activate_object();
    active_objects_composite_->deactivate_object();
    active_objects_composite_->wait_object();
  }
}

int
main()
{
  std::cout << "ActivateDeactivatePlanner functional test started.."
    << std::endl;
  try
  {
    ActivateDeactivatePlanner tester;
    tester.test();
    std::cout << "SUCCESS" << std::endl;
    return 0;
  }
  catch (const eh::Exception& e)
  {
    std::cerr << "FAIL std: " << e.what() << std::endl;
  }
  catch (...)
  {
    std::cerr << "FAIL: unknown exception raised" << std::endl;
  }

  return -1;
}
