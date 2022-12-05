#ifndef _CORBA_CRASHCALL_APPLICATION_HPP_
#define _CORBA_CRASHCALL_APPLICATION_HPP_

#include <eh/Exception.hpp>

#include <CORBACommons/ProcessControlImpl.hpp>
#include <CORBACommons/CorbaAdapters.hpp>

#include "TestCrash_s.hpp"


namespace CORBATest
{
  class TestCrashImpl :
    virtual public CORBACommons::ReferenceCounting::ServantImpl<
      POA_CORBATest::TestCrash>
  {
  public:
    virtual void
    crash() throw ();

  protected:
    virtual
    ~TestCrashImpl() throw ();
  };
  typedef ReferenceCounting::QualPtr<TestCrashImpl> TestCrashImpl_var;
}

inline
CORBATest::TestCrashImpl::~TestCrashImpl() throw ()
{
}

inline void
CORBATest::TestCrashImpl::crash() throw ()
{
  _exit(1);
}

class Application :
  public CORBACommons::ProcessControlImpl
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  Application() /*throw (eh::Exception)*/;

  void
  run(int argc, char* argv[]) /*throw (Exception, eh::Exception)*/;

protected:
  virtual
  ~Application() throw ();
};

#endif
