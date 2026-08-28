#include <iostream>

#include <ReferenceCounting/ReferenceCounting.hpp>

#include <Generics/ActiveObject.hpp>
#include <Generics/Singleton.hpp>
#include <Generics/Scheduler.hpp>


struct Simple
{
  Simple() noexcept;
  ~Simple() noexcept;
};

Simple::Simple() noexcept
{
  std::cout << "Simple::Simple()" << std::endl;
}

Simple::~Simple() noexcept
{
  std::cout << "Simple::~Simple()" << std::endl;
}

class RC : public ReferenceCounting::AtomicImpl
{
public:
  RC() noexcept;

protected:
  ~RC() noexcept;
};

RC::RC() noexcept
{
  std::cout << "RC::RC()" << std::endl;
}

RC::~RC() noexcept
{
  std::cout << "RC::~RC()" << std::endl;
}

class ActiveContainer
{
public:
  ActiveContainer(const char* kind = "singleton") /*throw (eh::Exception)*/;
  virtual ~ActiveContainer() noexcept;

private:
  class Callback :
    public Generics::ActiveObjectCallback,
    public ReferenceCounting::AtomicImpl
  {
  public:
    virtual void
    report_error(Severity severity, const String::SubString& description,
      const char* error_code = 0) noexcept;

    virtual void on_start() noexcept;

    virtual void on_stop() noexcept;

  protected:
    virtual ~Callback() noexcept;
  };
  const char* const KIND_;
  Generics::Planner_var active_object_;
};

ActiveContainer::ActiveContainer(const char* kind) /*throw (eh::Exception)*/
  : KIND_(kind),
    active_object_(new Generics::Planner( Generics::ActiveObjectCallback_var(new Callback)))
{
  std::cout << "ActiveContainer::ActiveContainer() " << KIND_ << std::endl;
  active_object_->activate_object();
}

ActiveContainer::~ActiveContainer() noexcept
{
  std::cout << "ActiveContainer::~ActiveContainer() " << KIND_ << std::endl;
  active_object_->deactivate_object();
  active_object_->wait_object();
}

void ActiveContainer::Callback::on_start() noexcept
{
  std::cout << "Started thread " << pthread_self() << std::endl;
}

void ActiveContainer::Callback::on_stop() noexcept
{
  std::cout << "Stopping thread " << pthread_self() << std::endl;
}

void
ActiveContainer::Callback::report_error(Severity /*severity*/,
  const String::SubString& /*description*/,
  const char* /*error_code*/) noexcept
{
}

ActiveContainer::Callback::~Callback() noexcept
{
}

ActiveContainer ac("static");

int main()
{
  ActiveContainer ac("auto");
  Generics::Singleton<ActiveContainer>::instance();
  Generics::Singleton<Simple>::instance();
  Generics::Singleton<RC, ReferenceCounting::QualPtr<RC> >::instance();
  return 0;
}
