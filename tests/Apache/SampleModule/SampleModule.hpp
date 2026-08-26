#pragma once

#include <ReferenceCounting/ReferenceCounting.hpp>

#include <Apache/Adapters.hpp>


class TestModule :
  public Apache::ConfigParser,
  public Apache::HandlerHook<TestModule>,
  public Apache::QuickHandlerAdapter<TestModule>,
  public Apache::ChildLifecycleAdapter<TestModule>,
  public ReferenceCounting::AtomicImpl
{
public:
  TestModule() /*throw (eh::Exception)*/;

  typedef ReferenceCounting::QualPtr<TestModule> TestModule_var;
  static TestModule_var instance;

  virtual int
  handler(request_rec *r) noexcept;
  virtual const char*
  handle_command(const ConfigArgs& args) noexcept;

  virtual bool
  will_handle(const char* uri) noexcept;
  virtual int
  handle_request(const Apache::HttpRequest& request,
    Apache::HttpResponse& response) noexcept;

  virtual void
  init() noexcept;
  virtual void
  shutdown() noexcept;

protected:
  virtual
  ~TestModule() noexcept;

private:
  int test_;
};
