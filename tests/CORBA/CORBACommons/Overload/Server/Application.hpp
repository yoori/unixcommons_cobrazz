#ifndef TEST_CORBA_OVERLOAD_SERVER_APPLICATION_HPP
#define TEST_CORBA_OVERLOAD_SERVER_APPLICATION_HPP

#include <eh/Exception.hpp>

#include <CORBACommons/ProcessControlImpl.hpp>
#include <CORBACommons/CorbaAdapters.hpp>

#include "TestIntImpl.hpp"


class Application:
  public CORBACommons::ProcessControlImpl,
  private CORBATest::TestIntImpl::Callback
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  Application() /*throw (eh::Exception)*/;

  void
  run(int argc, char* argv[]) /*throw (Exception, eh::Exception)*/;

  void
  error(const char* message) throw ();

  virtual char*
  control(const char* param_name, const char* param_value) throw ();

protected:
  virtual
  ~Application() throw ();

private:
  bool error_state_;
};

#endif
