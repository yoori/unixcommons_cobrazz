#ifndef CORBACOMMONS_ADAPTERS_INTERNAL_HPP
#define CORBACOMMONS_ADAPTERS_INTERNAL_HPP

#include <vector>

#include <Logger/Logger.hpp>

#include <CORBACommons/CorbaAdapters.hpp>


namespace CORBACommons
{
  namespace SSLData
  {
    DECLARE_EXCEPTION(FileError, eh::DescriptiveException);

    /**
     * Performs loading of PEM files
     * @param filename file with PEM data
     * @return content of the file
     */
    std::string
    load(const char* filename) /*throw (eh::Exception, FileError)*/;
  }

  namespace PropertiesHandling
  {
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    typedef std::vector<char*> SimpleORBProperties;

    void
    create_common_properties(ORBProperties& properties, bool custom_reactor)
      /*throw (eh::Exception)*/;
    void
    create_secure_properties(ORBProperties& properties,
      const SecureConnectionConfig& secure_connection_config)
      /*throw (eh::Exception, Exception)*/;
    int
    create_simple_properties(const ORBProperties& properties,
      SimpleORBProperties& simple_properties) /*throw (eh::Exception)*/;

    template<typename OStream>
    void
    print_properties(const ORBProperties& properties,
      OStream& ostr) /*throw (eh::Exception)*/;
  };

  class OrbCreator
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    static
    CORBA::ORB_ptr
    create_orb(const CORBACommons::ORBProperties& properties,
      const char* orb_id,
      const CORBACommons::SecureConnectionConfig*
        secure_connection_config = 0, const Generics::Time& timeout =
        Generics::Time::ZERO)
      /*throw (Exception, eh::Exception)*/;

  private:
    static
    int
    pem_password_callback_(char* buf, int size, int rwflag, void* userdata)
      noexcept;

    static
    void
    load_trusted_ca_(void* ctx, const char* file)
      /*throw (Exception, eh::Exception)*/;

    static Sync::PosixMutex mutex_;
    static std::string password_;
  };

  namespace AceLogger
  {
    void
    add_logger(Logging::Logger* logger) /*throw (eh::Exception)*/;
    void
    remove_logger(Logging::Logger* logger) noexcept;
  }

  static const unsigned DESCRIPTORS = 65536;

  static const unsigned PARTS = 8;
  static_assert(!(CORBACommons::PARTS & (PARTS - 1)),
    "PARTS is not a power of 2");
}

namespace CORBACommons::PropertiesHandling
{
  template<typename OStream>
  void
  print_properties(const ORBProperties& properties,
    OStream& ostr) /*throw (eh::Exception)*/
  {
    for (const auto& property: properties)
    {
      ostr << " '" << property.c_str() << "'";
    }
  }
}


#define TAO_LIB(x) ACE_DLL_PREFIX x ACE_DLL_SUFFIX "." TAO_VERSION

#endif
