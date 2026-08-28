#pragma once

#include <vector>

#include <Logger/Logger.hpp>

#include <CORBACommons/CorbaAdapters.hpp>


namespace CORBACommons::SSLData
{
  DECLARE_EXCEPTION(FileError, eh::DescriptiveException);

  /**
   * Performs loading of PEM files
   * @param filename file with PEM data
   * @return content of the file
   */
  std::string load(const char* filename) /*throw (eh::Exception, FileError)*/;
}

namespace CORBACommons::PropertiesHandling
{
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  using SimpleORBProperties = std::vector<char*>;

  void create_common_properties(ORBProperties& properties, bool custom_reactor)
    /*throw (eh::Exception)*/;
  void
  create_secure_properties(ORBProperties& properties,
    const SecureConnectionConfig& secure_connection_config)
    /*throw (eh::Exception, Exception)*/;
  int
  create_simple_properties(const ORBProperties& properties,
    SimpleORBProperties& simple_properties) /*throw (eh::Exception)*/;
  void print_properties(const ORBProperties& argv, std::ostream& ostr)
    /*throw (eh::Exception)*/;
}

namespace CORBACommons
{
  class OrbCreator
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    static
    CORBA::ORB_ptr
    create_orb(const CORBACommons::ORBProperties& properties,
      const char* orb_id,
      const CORBACommons::SecureConnectionConfig*
        secure_connection_config = 0, const Generics::Time& timeout = Generics::Time::ZERO)
      /*throw (Exception, eh::Exception)*/;

  private:
    static int pem_password_callback_(char* buf, int size, int rwflag, void* userdata) noexcept;

    static void load_trusted_ca_(void* ctx, const char* file)
      /*throw (Exception, eh::Exception)*/;

    static Sync::PosixMutex mutex_;
    static std::string password_;
  };

}

namespace CORBACommons::AceLogger
{
  void add_logger(Logging::Logger* logger) /*throw (eh::Exception)*/;
  void remove_logger(Logging::Logger* logger) noexcept;
}

namespace CORBACommons
{
  static const unsigned DESCRIPTORS = 65536;

  static const unsigned PARTS = 8;
  static_assert(!(CORBACommons::PARTS & (PARTS - 1)), "PARTS is not a power of 2");
}

#define TAO_LIB(x) ACE_DLL_PREFIX x ACE_DLL_SUFFIX "." TAO_VERSION
