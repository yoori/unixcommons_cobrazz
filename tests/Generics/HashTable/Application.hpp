/**
 * @file   Application.hpp
 * @author Karen Aroutiounov
 */

#ifndef GENERICS_HASHTABLE_APPLICATION_HPP
#define GENERICS_HASHTABLE_APPLICATION_HPP

#include <eh/Exception.hpp>
#include <Generics/Statistics.hpp>


namespace Generics
{
  class Application
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(InvalidArgument, Exception);
    DECLARE_EXCEPTION(InvalidOperationOrder, Exception);

  public:
    Application() /*throw (eh::Exception)*/;

    virtual
    ~Application() throw ();

    void
    init(int& argc, char** argv)
      /*throw (InvalidArgument, Exception, eh::Exception)*/;

    void
    run() /*throw (InvalidOperationOrder, Exception, eh::Exception)*/;

    bool
    active() /*throw (eh::Exception)*/;
    void
    stop() /*throw (Exception, eh::Exception)*/;

  private:

    void
    print_results() /*throw (eh::Exception)*/;

    void
    test() /*throw (Exception, eh::Exception)*/;
    void
    test_iteration() /*throw (Exception, eh::Exception)*/;

    void
    test_string_table() /*throw (Exception, eh::Exception)*/;
    void
    test_long_table() /*throw (Exception, eh::Exception)*/;
    void
    test_inserter_table() /*throw (Exception, eh::Exception)*/;
    void
    test_inserter_set() /*throw (Exception, eh::Exception)*/;

  private:
    typedef Sync::PosixRWLock Mutex_;
    typedef Sync::PosixRGuard Read_Guard_;
    typedef Sync::PosixWGuard Write_Guard_;

    mutable Mutex_ lock_;

    bool active_;
    Generics::Time execution_time_;

    Generics::Time start_time_;
    Generics::Time stop_time_;

    Generics::ActiveObjectCallback_var callback_;
    Statistics::Collection_var statistics_;
  };
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace Generics
{
  //
  // Application class
  //

  inline bool
  Application::active() /*throw (eh::Exception)*/
  {
    Read_Guard_ guard(lock_);
    return active_;
  }
}

#endif
