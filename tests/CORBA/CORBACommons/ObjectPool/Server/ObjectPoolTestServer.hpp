#pragma once

#include <cmath>
#include <eh/Exception.hpp>

#include <CORBACommons/ProcessControlImpl.hpp>
#include <CORBACommons/CorbaAdapters.hpp>

#include "Simple_s.hpp"

namespace CORBATest
{
  class TestObjectPoolImpl :
    public CORBACommons::ReferenceCounting::ServantImpl<
      POA_CORBATest::TestObjectPool>
  {
    static volatile _Atomic_word stat_counter_;
    volatile _Atomic_word counter_;
    std::size_t my_number_;
  public:
    TestObjectPoolImpl() noexcept;

    virtual ::CORBA::Long square(::CORBA::Long num) noexcept;

    virtual ::CORBA::Long root(::CORBA::Long num) noexcept;

    virtual CORBA::Long get_calling_number() noexcept;

    virtual void up() noexcept;

  protected:
    virtual ~TestObjectPoolImpl() noexcept;
  };
  using TestObjectPoolImpl_var = ReferenceCounting::QualPtr<
    TestObjectPoolImpl>;


  class PoolObjectImpl :
    public CORBACommons::ReferenceCounting::ServantImpl<
      POA_CORBATest::PoolObject>
  {
  public:
    virtual ::CORBA::Long is_base() noexcept;

  protected:
    virtual ~PoolObjectImpl() noexcept;
  };
  using PoolObjectImpl_var = ReferenceCounting::QualPtr<
    PoolObjectImpl>;

}

class Application :
  public CORBACommons::ProcessControlImpl
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  Application() /*throw (eh::Exception)*/;
  virtual ~Application() noexcept {}

  /**
   * Method up shutdown CORBA server and we starting new server
   * for the second time, but with with Cuatro object already.
   * @param after_up true if server should support Cuatro object
   */
  void run(int argc, char* argv[], std::size_t before_up = 1)
    /*throw (Exception, eh::Exception)*/;

  void create_names(std::size_t port, std::size_t count = 3)
    /*throw (eh::Exception)*/;

  static CORBACommons::OrbShutdowner_var shuter;
private:
  using ObjectNames = std::vector<std::string>;
  ObjectNames servants;
};

//////////////////////////////////////////////////////////////////////////
// Inlines implementations
//////////////////////////////////////////////////////////////////////////

namespace CORBATest
{
  //
  // TestObjectPoolImpl class
  //

  inline TestObjectPoolImpl::TestObjectPoolImpl() noexcept :
    counter_(0)
  {
    my_number_ = __gnu_cxx::__exchange_and_add(&stat_counter_, 1);
  }

  inline TestObjectPoolImpl::~TestObjectPoolImpl() noexcept
  {
  }

  inline ::CORBA::Long TestObjectPoolImpl::square(::CORBA::Long num) noexcept
  {
    __gnu_cxx::__atomic_add(&counter_, 1);
    return num * num;
  }

  inline ::CORBA::Long TestObjectPoolImpl::root(::CORBA::Long num) noexcept
  {
    __gnu_cxx::__atomic_add(&counter_, 1);
    return static_cast<long>(std::sqrt(num));
  }

  inline ::CORBA::Long TestObjectPoolImpl::get_calling_number() noexcept
  {
    ::CORBA::Long old = counter_;
    counter_ = 0;
    return old;
  }

  inline void TestObjectPoolImpl::up() noexcept
  {
    try
    {
      if (Application::shuter.in())
      {
        try
        {
          std::cout << "Shutting DOWN" << std::endl;
          Application::shuter->shutdown(false);
          std::cout << "Shut DOWN" << std::endl;
        }
        catch (...)
        {
        }
      }
    }
    catch (...)
    {
      // nothing to do for now
    }
  }

  inline PoolObjectImpl::~PoolObjectImpl() noexcept
  {
  }

  inline ::CORBA::Long PoolObjectImpl::is_base() noexcept
  {
    return 12345;
  }


}
