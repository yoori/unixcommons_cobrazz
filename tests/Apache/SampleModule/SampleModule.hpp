/**
 * @author Pavel Gubin <pgubin@ipmce.ru>
 */
#ifndef APACHE_SAMPLE_MODULE_HPP
#define APACHE_SAMPLE_MODULE_HPP

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
  handler(request_rec *r) throw ();
  virtual const char*
  handle_command(const ConfigArgs& args) throw ();

  virtual bool
  will_handle(const char* uri) throw ();
  virtual int
  handle_request(const Apache::HttpRequest& request,
    Apache::HttpResponse& response) throw ();

  virtual void
  init() throw ();
  virtual void
  shutdown() throw ();

protected:
  virtual
  ~TestModule() throw ();

private:
  int test_;
};

#endif // _SAMPLE_MODULE_HPP_
