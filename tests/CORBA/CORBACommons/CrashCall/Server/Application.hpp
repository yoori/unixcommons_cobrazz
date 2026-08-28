#pragma once

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
    virtual void crash() noexcept;

  protected:
    virtual ~TestCrashImpl() noexcept;
  };
  using TestCrashImpl_var = ReferenceCounting::QualPtr<TestCrashImpl>;
}

inline CORBATest::TestCrashImpl::~TestCrashImpl() noexcept
{
}

inline void CORBATest::TestCrashImpl::crash() noexcept
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

  void run(int argc, char* argv[]) /*throw (Exception, eh::Exception)*/;

protected:
  virtual ~Application() noexcept;
};
