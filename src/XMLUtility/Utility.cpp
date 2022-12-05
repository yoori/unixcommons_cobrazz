/**
 * @file   Utility.cpp
 * @author Karen Aroutiounov <karen@ipmce.ru>
 */

#include <sstream>
#include <string>

#include <xercesc/util/PlatformUtils.hpp>

#include <eh/Exception.hpp>
#include <Sync/PosixLock.hpp>
#include <String/StringManip.hpp>
#include <Stream/MemoryStream.hpp>

#include <XMLUtility/Utility.hpp>


namespace XMLUtility
{
  typedef Sync::PosixMutex Mutex_;
  typedef Sync::PosixGuard Guard_;

  static Mutex_ lock_;
  static unsigned long init_counter_ = 0;

  void
  initialize() /*throw (Exception, eh::Exception)*/
  {
    Guard_ guard(lock_);

    if (!init_counter_)
    {
      try
      {
        XMLPlatformUtils::Initialize();
      }
      catch (const XMLException& e)
      {
        std::string msg;
        StringManip::xmlch_to_mbc(e.getMessage(), msg);

        Stream::Error ostr;
        ostr << "XMLUtility::initialize(): "
          "XMLException thrown by XMLPlatformUtils::Initialize():" << msg;
        throw Exception(ostr);
      }
    }

    init_counter_++;
  }

  void
  terminate() throw ()
  {
    Guard_ guard(lock_);

    if (init_counter_)
    {
      if (!--init_counter_)
      {
        try
        {
          XMLPlatformUtils::Terminate();
        }
        catch (const XMLException& e)
        {
        }
      }
    }
  }
}
