#include <Generics/Periodic.hpp>

#include <Logger/StreamLogger.hpp>

Logging::QLogger_var out;

class PeriodicPrint : public Generics::PeriodicTask
{
public:
  PeriodicPrint(int time) throw ();

  virtual
  void
  task(bool forced) throw ();

protected:
  virtual
  ~PeriodicPrint() throw ();

private:
  int index_;
};


PeriodicPrint::PeriodicPrint(int time) throw ()
  : Generics::PeriodicTask(Generics::Time(time)), index_(time)
{
}

void
PeriodicPrint::task(bool forced) throw ()
{
  out->stream(Logging::Logger::INFO) << index_ << (forced ? " forced" : "");
}

PeriodicPrint::~PeriodicPrint() throw ()
{
}

void
test1() /*throw (eh::Exception)*/
{
  const int N = 4;
  Generics::PeriodicTask_var tasks[N];
  Generics::PeriodicRunner_var pr(
    new Generics::PeriodicRunner(0));
  for (int i = 0; i < N; i++)
  {
    pr->add_task(tasks[i] = new PeriodicPrint(i + 1), true);
  }

  out->stream(Logging::Logger::INFO) << "forward";

  pr->activate_object();

  sleep(10);

  out->stream(Logging::Logger::INFO) << "backward";
  for (int i = 0; i < N; i++)
  {
    tasks[i]->set_period(Generics::Time(N - i));
  }
  sleep(10);

  out->stream(Logging::Logger::INFO) << "force all";
  for (int i = 0; i < 3; i++)
  {
    pr->enforce_start_all();
    sleep(1);
  }

  out->stream(Logging::Logger::INFO) << "force 0";
  for (int i = 0; i < 5; i++)
  {
    tasks[0]->enforce_start();
  }
  sleep(1);

  pr->deactivate_object();
  pr->wait_object();

}

int
main()
{
  std::cout << "Periodic tests started.." << std::endl;
  try
  {
    out = new Logging::OStream::Logger(Logging::OStream::Config(std::cout));
    test1();
    std::cout << "SUCCESS" << std::endl;
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << "FAIL: " << ex.what() << std::endl;
  }
  catch (...)
  {
    std::cerr << "FAIL: unknown exception raised" << std::endl;
  }

  return 0;
}
