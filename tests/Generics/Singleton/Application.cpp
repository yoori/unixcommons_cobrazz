#include <iostream>

#include <ReferenceCounting/ReferenceCounting.hpp>

#include <Generics/ActiveObject.hpp>
#include <Generics/Singleton.hpp>
#include <Generics/Scheduler.hpp>


struct Simple
{
  Simple() throw ();
  ~Simple() throw ();
};

Simple::Simple() throw ()
{
  std::cout << "Simple::Simple()" << std::endl;
}

Simple::~Simple() throw ()
{
  std::cout << "Simple::~Simple()" << std::endl;
}

class RC : public ReferenceCounting::AtomicImpl
{
public:
  RC() throw ();

protected:
  ~RC() throw ();
};

RC::RC() throw ()
{
  std::cout << "RC::RC()" << std::endl;
}

RC::~RC() throw ()
{
  std::cout << "RC::~RC()" << std::endl;
}

class ActiveContainer
{
public:
  ActiveContainer(const char* kind = "singleton") /*throw (eh::Exception)*/;
  virtual
  ~ActiveContainer() throw ();

private:
  class Callback :
    public Generics::ActiveObjectCallback,
    public ReferenceCounting::AtomicImpl
  {
  public:
    virtual void
    report_error(Severity severity, const String::SubString& description,
      const char* error_code = 0) throw ();

    virtual
    void
    on_start() throw ();

    virtual
    void
    on_stop() throw ();

  protected:
    virtual
    ~Callback() throw ();
  };
  const char* const KIND_;
  Generics::Planner_var active_object_;
};

ActiveContainer::ActiveContainer(const char* kind) /*throw (eh::Exception)*/
  : KIND_(kind),
    active_object_(new Generics::Planner(
      Generics::ActiveObjectCallback_var(new Callback)))
{
  std::cout << "ActiveContainer::ActiveContainer() " << KIND_ << std::endl;
  active_object_->activate_object();
}

ActiveContainer::~ActiveContainer() throw ()
{
  std::cout << "ActiveContainer::~ActiveContainer() " << KIND_ << std::endl;
  active_object_->deactivate_object();
  active_object_->wait_object();
}

void
ActiveContainer::Callback::on_start() throw ()
{
  std::cout << "Started thread " << pthread_self() << std::endl;
}

void
ActiveContainer::Callback::on_stop() throw ()
{
  std::cout << "Stopping thread " << pthread_self() << std::endl;
}

void
ActiveContainer::Callback::report_error(Severity /*severity*/,
  const String::SubString& /*description*/,
  const char* /*error_code*/) throw ()
{
}

ActiveContainer::Callback::~Callback() throw ()
{
}

ActiveContainer ac("static");

int
main()
{
  ActiveContainer ac("auto");
  Generics::Singleton<ActiveContainer>::instance();
  Generics::Singleton<Simple>::instance();
  Generics::Singleton<RC, ReferenceCounting::QualPtr<RC> >::instance();
  return 0;
}
