/**
 * @file   ProbeObj/Application.hpp
 * @author Karen Aroutiounov [karen@peopleonpage.com]
 * Declares class which tests and shutdowns CORBA servers
 */

#ifndef CORBA_PROBE_OBJ_APPLICATION_HPP
#define CORBA_PROBE_OBJ_APPLICATION_HPP

#include <eh/Exception.hpp>

#include <CORBACommons/CorbaAdapters.hpp>


/**
 * Class which tests and shutdowns CORBA server processes by means of
 * CORBACommons::IProcessControl interface usage.
 */
class Application
{
public:
  /**
   * Application base exception class.
   */
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  /**
   * Macros defining InvalidArgument exception class.
   */
  DECLARE_EXCEPTION(InvalidArgument, Exception);
  /**
   * Macros defining InvalidReference exception class.
   */
  DECLARE_EXCEPTION(InvalidReference, Exception);

public:
  /**
   * Tests or shutdowns CORBA server processes.
   * @param argc Number of arguments passed to utility process
   * @param argv Arguments passed to utility process
   */
  int
  run(int& argc, char** argv)
    /*throw (InvalidArgument, InvalidReference, Exception, eh::Exception,
      CORBA::Exception)*/;

private:
  int
  shutdown_(int argc, char** argv)
    /*throw (InvalidArgument, InvalidReference, Exception, eh::Exception,
      CORBA::Exception)*/;

  int
  probe_(int argc, char** argv)
    /*throw (InvalidArgument, InvalidReference, Exception, eh::Exception,
      CORBA::Exception)*/;

  int
  status_(int argc, char** argv)
    /*throw (InvalidArgument, InvalidReference, Exception, eh::Exception,
      CORBA::Exception)*/;

  int
  control_(int argc, char** argv)
    /*throw (InvalidArgument, InvalidReference, Exception, eh::Exception,
      CORBA::Exception)*/;


  Logging::Logger_var logger_;
  CORBACommons::CorbaClientAdapter_var adapter_;
};

#endif
