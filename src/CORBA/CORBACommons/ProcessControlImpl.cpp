/**
 * @file   CORBACommons/ProcessControlImpl.cpp
 * @author Karen Aroutiounov [karen@peopleonpage.com]
 * Implements class for CORBA process control
 */

#include <CORBACommons/ProcessControlImpl.hpp>


namespace CORBACommons
{
  //
  // OrbShutdowner class
  //

  OrbShutdowner::OrbShutdowner() throw ()
  {
  }

  OrbShutdowner::~OrbShutdowner() throw ()
  {
  }


  //
  // SimpleOrbShutdowner class
  //

  SimpleOrbShutdowner::SimpleOrbShutdowner(CORBA::ORB_ptr orb) throw ()
    : orb_(CORBA::ORB::_duplicate(orb))
  {
  }

  SimpleOrbShutdowner::~SimpleOrbShutdowner() throw ()
  {
  }

  void
  SimpleOrbShutdowner::shutdown(bool type) throw ()
  {
    if (!CORBA::is_nil(orb_))
    {
      try
      {
        orb_->shutdown(type);
      }
      catch (...)
      {
      }
    }
  }


  //
  // ProcessControlImpl class
  //

  ProcessControlImpl::ProcessControlImpl(OrbShutdowner* shutdowner)
    /*throw (InvalidArgument, Exception, eh::Exception)*/
    : shutdowner_(::ReferenceCounting::add_ref(shutdowner)),
      job_(new ShutdownJob(shutdowner_)), thread_runner_(job_, 1)
  {
    try
    {
      thread_runner_.start();
    }
    catch (const eh::Exception& e)
    {
      Stream::Error ostr;
      ostr << FNS << "eh::Exception caught:" << e.what();
      throw Exception(ostr);
    }
  }

  ProcessControlImpl::~ProcessControlImpl() throw ()
  {
    job_->wake(false);
  }

  void
  ProcessControlImpl::wait() /*throw (eh::Exception)*/
  {
    thread_runner_.wait_for_completion();
  }

  void
  ProcessControlImpl::shutdown(CORBA::Boolean wait_for_completion)
    /*throw (CORBA::SystemException)*/
  {
    try
    {
      if (shutdowner_)
      {
        if (wait_for_completion)
        {
          job_->wake(true);
        }
        else
        {
          job_->wake(false);
          shutdowner_->shutdown(false);
        }
      }
    }
    catch (...)
    {
      // nothing to do for now
    }
  }


  //
  // ProcessControlImpl::ShutdownJob class
  //

  ProcessControlImpl::ShutdownJob::ShutdownJob(
    OrbShutdowner_var& shutdowner) throw ()
    : shutdowner_(shutdowner), sem_(0)
  {
  }

  ProcessControlImpl::ShutdownJob::~ShutdownJob() throw ()
  {
  }

  void
  ProcessControlImpl::ShutdownJob::work() throw ()
  {
    sem_.acquire();
    if (shutdown_ && shutdowner_)
    {
      shutdowner_->shutdown(true);
    }
  }

  void
  ProcessControlImpl::ShutdownJob::wake(bool shutdown) throw ()
  {
    shutdown_ = shutdown;
    sem_.release();
  }
}
