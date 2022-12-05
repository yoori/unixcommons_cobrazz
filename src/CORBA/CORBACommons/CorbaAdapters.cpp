// CorbaAdapters.cpp

#include <sstream>

#include <ace/SSL/SSL_Context.h>

#include <tao/Messaging/Messaging.h>

#include <eh/Errno.hpp>

#include "CorbaAdaptersInternal.hpp"


namespace
{
  //
  // ORBIdGenerator
  //

  struct ORBIdGenerator
  {
    const char*
    generate(const char* prefix, std::string& id) /*throw (eh::Exception)*/
    {
      WriteGuard_ lock(lock_);

      NameCountMap::iterator it = name_count_map_.find(prefix);
      if (it == name_count_map_.end())
      {
        it = name_count_map_.insert(
          std::make_pair(std::string(prefix), 0)).first;
      }

      Stream::Stack<1024> ostr;
      ostr << prefix << "_" << it->second++;
      ostr.str().assign_to(id);

      return id.c_str();
    }

    typedef std::map<std::string, unsigned int> NameCountMap;
    typedef Sync::PosixMutex Mutex_;
    typedef Sync::PosixGuard WriteGuard_;

    Mutex_ lock_;
    NameCountMap name_count_map_;

    static ORBIdGenerator instance;
  };

  ORBIdGenerator ORBIdGenerator::instance;

  // callbacks holder
  struct BIOWrap
  {
    int (*bwrite)(BIO* bio, const char* buf, int size);
    int (*bread)(BIO* bio, char* buf, int size);
    int (*bputs)(BIO* bio, const char* str);
    int (*bgets)(BIO* bio, char* str, int size);
    long (*ctrl)(BIO* bio, int command, long arg1, void* arg2);
    int (*create)(BIO* bio);
    int (*destroy)(BIO* bio);

    const BIO_METHOD* bio_method;
  };

  //
  // SSL BIO enhancer
  //

  class BIOEnhancer
  {
  public:
    BIOEnhancer() /*throw (eh::Exception)*/;

    const BIO_METHOD*
    intercept_bio_meth() const;

  private:
    static
    int
    write(BIO*, const char*, int) throw ();

    static
    int
    read(BIO*, char*, int) throw ();

    static
    int
    puts(BIO*, const char*) throw ();

    static
    int
    gets(BIO*, char*, int) throw ();

    static
    long
    ctrl(BIO*, int, long, void*) throw ();

    static
    int
    create(BIO*) throw ();

    static
    int
    destroy(BIO*) throw ();

    static
    long
    callback_ctrl(BIO*, int, bio_info_cb*) throw ();

    class DataFile : private Generics::Uncopyable
    {
    public:
      explicit
      DataFile(void* original_ptr) /*throw (eh::Exception)*/;

      bool
      is_initialized() const throw ();

      void
      original_pointer(void* original_ptr) throw ();

      void*
      original_pointer() const throw ();

      void
      assign(const char* data) /*throw (eh::Exception)*/;

      int
      gets(char* buf, int size) throw ();

    private:
      void* original_ptr_;
      std::string data_;
      std::string::size_type position_;
    };

    class StorageGuard : private Generics::Uncopyable
    {
    public:
      explicit
      StorageGuard(BIO* bio) throw ();

      ~StorageGuard() throw ();

      DataFile*
      operator ->() throw ();

    private:
      BIO* bio_;
      DataFile* data_file_;
    };

    const BIO_METHOD* intercept_bio_meth_;
    static const BIO_METHOD* original_; // hold original file BIO methods
  };

  const BIO_METHOD* BIOEnhancer::original_;

  BIOEnhancer::DataFile::DataFile(void* original_ptr) /*throw (eh::Exception)*/
    : original_ptr_(original_ptr), position_(0)
  {
  }

  bool
  BIOEnhancer::DataFile::is_initialized() const throw ()
  {
    return !data_.empty();
  }

  void
  BIOEnhancer::DataFile::original_pointer(void* original_ptr) throw ()
  {
    original_ptr_ = original_ptr;
  }

  void*
  BIOEnhancer::DataFile::original_pointer() const throw ()
  {
    return original_ptr_;
  }

  void
  BIOEnhancer::DataFile::assign(const char* key) /*throw (eh::Exception)*/
  {
    String::StringManip::mime_url_decode(String::SubString(key + 1), data_);
  }

  int
  BIOEnhancer::DataFile::gets(char* buf, int size) throw ()
  {
    if (size <= 0)
    {
      return 0;
    }
    char* ptr = buf;
    while (--size > 0 && position_ < data_.size() &&
      (*ptr++ = data_[position_++]) != '\n');
    *ptr = '\0';
    return ptr - buf;
  }

  BIOEnhancer::StorageGuard::StorageGuard(BIO* bio) throw ()
    : bio_(bio),
      data_file_(static_cast<DataFile*>(BIO_get_data(bio_)))
  {
    BIO_set_data(bio_, data_file_->original_pointer());
  }

  BIOEnhancer::StorageGuard::~StorageGuard() throw ()
  {
    data_file_->original_pointer(BIO_get_data(bio_));
    BIO_set_data(bio_, data_file_);
  }

  BIOEnhancer::DataFile*
  BIOEnhancer::StorageGuard::operator ->() throw ()
  {
    return data_file_;
  }

  BIOEnhancer::BIOEnhancer() /*throw (eh::Exception)*/
  {
    // save pointers to original bio methods
    original_ = BIO_s_file();

    // create intercept methods
    BIO_METHOD* intercept_bio_meth = BIO_meth_new(
      BIO_TYPE_SOURCE_SINK | BIO_get_new_index(), "CorbaAdapters::BIOEnhancer");
    BIO_meth_set_write(intercept_bio_meth, write);
    BIO_meth_set_read(intercept_bio_meth, read);
    BIO_meth_set_puts(intercept_bio_meth, puts);
    BIO_meth_set_gets(intercept_bio_meth, gets);
    BIO_meth_set_ctrl(intercept_bio_meth, ctrl);
    BIO_meth_set_create(intercept_bio_meth, create);
    BIO_meth_set_destroy(intercept_bio_meth, destroy);

    intercept_bio_meth_ = intercept_bio_meth;
  }

  const BIO_METHOD*
  BIOEnhancer::intercept_bio_meth() const
  {
    return intercept_bio_meth_;
  }

  int
  BIOEnhancer::write(BIO* bio, const char* buf, int size) throw ()
  {
    StorageGuard guard(bio);
    return guard->is_initialized() ? 0 : BIO_meth_get_write(original_)(bio, buf, size);
  }

  int
  BIOEnhancer::read(BIO* bio, char* buf, int size) throw ()
  {
    StorageGuard guard(bio);
    return guard->is_initialized() ? 0 : BIO_meth_get_read(original_)(bio, buf, size);
  }

  int
  BIOEnhancer::puts(BIO* bio, const char* str) throw ()
  {
    StorageGuard guard(bio);
    return guard->is_initialized() ? 0 : BIO_meth_get_puts(original_)(bio, str);
  }

  int
  BIOEnhancer::gets(BIO* bio, char* str, int size) throw ()
  {
    StorageGuard guard(bio);
    return guard->is_initialized() ? guard->gets(str, size) :
      BIO_meth_get_gets(original_)(bio, str, size);
  }

  long
  BIOEnhancer::ctrl(BIO* bio, int command, long arg1, void* arg2) throw ()
  {
    StorageGuard guard(bio);
    if (guard->is_initialized())
    {
      return 0;
    }
    if (command == BIO_C_SET_FILENAME && arg1 == (BIO_CLOSE | BIO_FP_READ) &&
      arg2 && *static_cast<const char*>(arg2) == ':')
    {
      try
      {
        guard->assign(static_cast<const char*>(arg2));
        BIO_set_init(bio, 1);
      }
      catch (...)
      {
        return 0;
      }
      return 1;
    }
    return BIO_meth_get_ctrl(original_)(bio, command, arg1, arg2);
  }

  int
  BIOEnhancer::create(BIO* bio) throw ()
  {
    if (!BIO_meth_get_create(original_)(bio))
    {
      return 0;
    }
    try
    {
      DataFile* data_file = new DataFile(BIO_get_data(bio));
      BIO_set_data(bio, data_file);
    }
    catch (...)
    {
      return 0;
    }
    return 1;
  }

  int
  BIOEnhancer::destroy(BIO* bio) throw ()
  {
    int res;
    {
      StorageGuard guard(bio);
      res = BIO_meth_get_destroy(original_)(bio);
    }
    delete static_cast<DataFile*>(BIO_get_data(bio));
    BIO_set_data(bio, 0);
    return res;
  }

  BIOEnhancer bio_enhancer;
}

namespace CORBACommons
{
  //
  // Properties handling
  //

  void
  PropertiesHandling::create_common_properties(ORBProperties& properties,
    bool custom_reactor) /*throw (eh::Exception)*/
  {
    properties.emplace_back("-ORBGestalt");
    properties.emplace_back("Local");

    properties.emplace_back("-ORBKeepalive");
    properties.emplace_back("1");

    properties.emplace_back("-ORBUseLocalMemoryPool");
    properties.emplace_back("0");

    properties.emplace_back("-ORBCollocation");
    properties.emplace_back("NO");

    properties.emplace_back("-ORBSvcConfDirective");
    properties.emplace_back("dynamic TAO_Transport_Current_Loader "
      "Service_Object * " TAO_LIB("TAO_TC")
      ":_make_TAO_Transport_Current_Loader() \"\"");

    properties.emplace_back("-ORBSvcConfDirective");
    properties.emplace_back("dynamic TAO_EndpointPolicy_Initializer "
      "Service_Object * " TAO_LIB("TAO_EndpointPolicy")
      ":_make_TAO_EndpointPolicy_Initializer() \"\"");

    properties.emplace_back("-ORBSvcConfDirective");
    properties.emplace_back("dynamic TAO_Transport_IIOP_Current_Loader "
      "Service_Object * " TAO_LIB("TAO_TC_IIOP")
      ":_make_TAO_Transport_IIOP_Current_Loader() \"\"");

    properties.emplace_back("-ORBSvcConfDirective");
    if (custom_reactor)
    {
      properties.push_back("static Resource_Factory "
        "\"-ORBProtocolFactory IIOP_Factory "
        "-ORBFlushingStrategy blocking "
        "-ORBCustomReactorImplFactory\"");
    }
    else
    {
      properties.emplace_back("static Resource_Factory "
        "\"-ORBProtocolFactory IIOP_Factory\"");
    }
  }

  namespace
  {
    Sync::PosixMutex zlib_lock;
    bool zlib_enabled = false;
  }

  void
  PropertiesHandling::create_secure_properties(ORBProperties& properties,
    const SecureConnectionConfig& secure_connection_config)
    /*throw (eh::Exception, Exception)*/
  {
    // Enable compression is SSL
    {
      Sync::PosixGuard guard(zlib_lock);
      if (!zlib_enabled)
      {
        COMP_METHOD* cm = COMP_zlib();
        if (!cm || COMP_get_type(cm) == NID_undef)
        {
          throw Exception("create_secure_properties(): "
            "SSL does not support zlib");
        }
        if (SSL_COMP_add_compression_method(255, cm))
        {
          throw Exception("create_secure_properties(): "
            "Failed to set zlib support for SSL");
        }
        zlib_enabled = true;
      }
    }

    properties.emplace_back("-ORBSvcConfDirective");
    {
      Stream::Dynamic ostr(4096);
      ostr << "dynamic SSLIOP_Factory Service_Object * "
        TAO_LIB("TAO_SSLIOP")
        ":_make_TAO_SSLIOP_Protocol_Factory() "
        "\"-SSLAuthenticate SERVER_AND_CLIENT -SSLPrivateKey PEM:" <<
        secure_connection_config.private_key <<
        " -SSLCertificate PEM:" <<
        secure_connection_config.own_certificate <<
        "\"";
      properties.emplace_back(ostr.str().str());
    }
    properties.emplace_back("-ORBSvcConfDirective");
    properties.emplace_back("static Resource_Factory "
      "\"-ORBProtocolFactory SSLIOP_Factory\"");
  }

  int
  PropertiesHandling::create_simple_properties(
    const ORBProperties& properties,
    SimpleORBProperties& simple_properties) /*throw (eh::Exception)*/
  {
    simple_properties.reserve(properties.size() + 1);
    for (ORBProperties::const_iterator itor =
      properties.begin(); itor != properties.end(); ++itor)
    {
      simple_properties.push_back(const_cast<char*>(itor->c_str()));
    }
    simple_properties.push_back(0);
    return properties.size();
  }

  void
  PropertiesHandling::print_properties(const ORBProperties& properties,
    std::ostream& ostr)
    /*throw (eh::Exception)*/
  {
    for (CORBACommons::ORBProperties::const_iterator itor(properties.begin());
      itor != properties.end(); ++itor)
    {
      ostr << " '" << itor->c_str() << "'";
    }
  }


  //
  // OrbCreator
  //

  Sync::PosixMutex OrbCreator::mutex_;
  std::string OrbCreator::password_;

  void
  OrbCreator::load_trusted_ca_(void* ctx, const char* file)
    /*throw (Exception, eh::Exception)*/
  {
    // It's a copy of X509_load_cert_crl_file but it uses
    // BIO_new(BIO_s_file()) and BIO_read_filename(in, file) instead of
    // BIO_new_file(file, "r")

    X509_LOOKUP* lookup = X509_STORE_add_lookup(
      static_cast<X509_STORE*>(ctx), X509_LOOKUP_file());
    if (!lookup)
    {
      Stream::Error ostr;
      ostr << FNS << "Failed to create lookup";
      throw Exception(ostr);
    }

    BIO* in = BIO_new(bio_enhancer.intercept_bio_meth());
    
    if (!in || BIO_read_filename(in, file) <= 0)
    {
      if (in)
      {
        BIO_free(in);
      }
      Stream::Error ostr;
      ostr << FNS << "Failed to open file '" << file << "'";
      throw Exception(ostr);
    }
    STACK_OF(X509_INFO)* inf = PEM_X509_INFO_read_bio(in, 0, 0, 0);
    BIO_free(in);
    if (!inf)
    {
      Stream::Error ostr;
      ostr << FNS << "Failed to find useful information in file '" <<
        file << "'";
      throw Exception(ostr);
    }
    for (int i = 0; i < sk_X509_INFO_num(inf); i++)
    {
      X509_INFO* itmp = sk_X509_INFO_value(inf, i);
      if (itmp->x509)
      {
        X509_STORE_add_cert(X509_LOOKUP_get_store(lookup), itmp->x509);
      }
      if (itmp->crl)
      {
        X509_STORE_add_crl(X509_LOOKUP_get_store(lookup), itmp->crl);
      }
    }
    sk_X509_INFO_pop_free(inf, X509_INFO_free);
  }

  CORBA::ORB_ptr
  OrbCreator::create_orb(const CORBACommons::ORBProperties& properties,
    const char* orb_id_prefix, const CORBACommons::SecureConnectionConfig*
      secure_connection_config, const Generics::Time& timeout)
    /*throw (Exception, eh::Exception)*/
  {
    std::string orb_id;
    ORBIdGenerator::instance.generate(orb_id_prefix, orb_id);

    PropertiesHandling::SimpleORBProperties args;
    int argc = PropertiesHandling::create_simple_properties(properties, args);

    Sync::PosixGuard guard(mutex_);

    if (secure_connection_config && secure_connection_config->is_secure())
    {
      SSL_CTX* ctx = ACE_SSL_Context::instance()->context();
      load_trusted_ca_(
        SSL_CTX_get_cert_store(ctx), 
        secure_connection_config->peer_certificate_authority);
      password_ = secure_connection_config->pass_phrase;
      SSL_CTX_set_default_passwd_cb(ctx, pem_password_callback_);
    }

    CORBA::ORB_var orb;

    for (;;)
    {
      try
      {
        orb = CORBA::ORB_init(argc, &args[0], orb_id.c_str());
        if (!CORBA::is_nil(orb))
        {
          break;
        }
      }
      catch (...)
      {
      }

      Stream::Error ostr;
      ostr << FNS << "Failed to create orb " << orb_id <<
        " with parameters '";
      PropertiesHandling::print_properties(properties, ostr);
      ostr << "'";
      throw Exception(ostr);
    }

    if (timeout != Generics::Time::ZERO)
    {
      CORBA::PolicyManager_var policy_manager =
        CORBA::PolicyManager::_narrow(
          orb->resolve_initial_references("ORBPolicyManager"));
      CORBA::Any timeout_as_any;
      timeout_as_any <<= TimeBase::TimeT(
        (timeout * Generics::Time::USEC_MAX).tv_sec * 10);
      CORBA::Policy_var policy =
        orb->create_policy(Messaging::RELATIVE_RT_TIMEOUT_POLICY_TYPE,
          timeout_as_any);
      CORBA::PolicyList policy_list(1);
      policy_list.length(1);
      policy_list[0] = policy;
      policy_manager->set_policy_overrides(policy_list, CORBA::ADD_OVERRIDE);
    }

    return orb._retn();
  }

  int
  OrbCreator::pem_password_callback_(char* buf, int size, int, void*)
    throw ()
  {
    return String::StringManip::strlcpy(buf, password_.c_str(), size);
  }

  namespace SSLData
  {
    std::string
    load(const char* filename) /*throw (eh::Exception, FileError)*/
    {
      int file = open(filename, O_RDONLY);
      if (file < 0)
      {
        eh::throw_errno_exception<FileError>(FNE,
          "Failed to open '", filename, "'");
      }

      char buf[16384];
      ssize_t got = ::read(file, buf, sizeof(buf) - 1);
      int error = errno;
      close(file);
      if (got < 0)
      {
        eh::throw_errno_exception<FileError>(error, FNE,
          "Failed to read '", filename, "'");
      }

      std::string encoded;
      String::StringManip::mime_url_encode(
        String::SubString(buf, got), encoded);
      return std::string(":") + encoded;
    }
  }


  //
  // SecureConnectionConfig class
  //

  SecureConnectionConfig::SecureConnectionConfig(const char* key_file,
    const char* pass_phrase, const char* certificate,
    const char* certificate_authority)
    /*throw (Exception)*/
  {
    parse(key_file, pass_phrase, certificate, certificate_authority);
  }

  void
  SecureConnectionConfig::parse(const char* key_file,
    const char* passphrase, const char* certificate_file,
    const char* certificate_authority_file)
    /*throw (Exception)*/
  {
    try
    {
      private_key << SSLData::load(key_file);
    }
    catch (const CORBA::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FNS << "Can't store file '" << key_file <<
        "' with secure key: " << ex;
      throw Exception(ostr);
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FNS << "Can't load file '" << key_file <<
        "' with secure key: " << ex.what();
      throw Exception(ostr);
    }

    pass_phrase = passphrase;

    try
    {
      own_certificate << SSLData::load(certificate_file);
    }
    catch (const CORBA::SystemException& ex)
    {
      Stream::Error ostr;
      ostr << FNS << "Can't store file '" << certificate_file <<
        "' with certificate: " << ex;
      throw Exception(ostr);
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FNS << "Can't load file '" << certificate_file <<
        "' with certificate: " << ex.what();
      throw Exception(ostr);
    }

    try
    {
      peer_certificate_authority <<
        SSLData::load(certificate_authority_file);
    }
    catch (const CORBA::SystemException& ex)
    {
      Stream::Error ostr;
      ostr << FNS << "Can't store file '" << certificate_authority_file <<
        "' with certificate authority: " << ex;
      throw Exception(ostr);
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FNS << "Can't load file '" << certificate_authority_file <<
        "' with certificate authority: " << ex.what();
      throw Exception(ostr);
    }
  }
}
