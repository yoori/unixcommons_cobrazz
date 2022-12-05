/**
 * @file   Application.hpp
 * @author Karen Aroutiounov [karen@peopleonpage.com]
 * Declares class which tests object of CORBACommons::ProcessControlImpl class
 */

#ifndef _TEST_PROCESS_CONTROL_APPLICATION_HPP_
#define _TEST_PROCESS_CONTROL_APPLICATION_HPP_

#include <eh/Exception.hpp>

#include <tao/ORB.h>
#include <tao/IORTable/IORTable.h>

#include <CORBACommons/ProcessControlImpl.hpp>


class Application
{
public:

  /**X
   * Macros defining Application base exception class.
   */
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  /**X
   * Macros defining InvalidArgument exception class.
   */
  DECLARE_EXCEPTION(InvalidArgument, Exception);

  /**X
   * Macros defining InvalidOperationOrder exception class.
   */
  DECLARE_EXCEPTION(InvalidOperationOrder, Exception);

public:

  /**X
   * Construct Application object.
   */
  Application() /*throw(Exception, eh::Exception)*/;

  /**X
   * Destructs Application object.
   */
  virtual ~Application() throw();

/**X
 * Initalizes ORB, POA, creates and
 * registers CORBACommons::ProcessControlImpl servant object.
 * @param argc Number of arguments passed to test process
 * @param argv Arguments passed to test process
 */
  void init(int& argc, char** argv)
    /*throw(InvalidArgument, Exception, eh::Exception)*/;

/**X
 * Runs ORB loop.
 */
  void run()
    /*throw(InvalidOperationOrder, Exception, eh::Exception)*/;

/**X
 * Destroys POA, ORB, release all resources.
 */
  void destroy() /*throw(eh::Exception)*/;


private:

  typedef Sync::PosixRWLock Mutex_;
  typedef Sync::PosixRGuard Read_Guard_;
  typedef Sync::PosixWGuard Write_Guard_;

  mutable Mutex_ lock_;

  CORBA::ORB_var orb_;
  PortableServer::POA_var server_poa_;
  CORBA::String_var process_control_name_;
  PortableServer::ObjectId_var process_control_id_;
  IORTable::Table_var ior_table_;
  PortableServer::ServantBase_var servant_;
};

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

#endif // _TEST_PROCESS_CONTROL_APPLICATION_HPP_
