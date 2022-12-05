#ifndef COMMONS_CORBACONFIGREADER_HPP
#define COMMONS_CORBACONFIGREADER_HPP

#include <CORBACommons/CorbaAdapters.hpp>


namespace CORBAConfigParser
{
  class CorbaConfigReader
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    static
    void
    read_server_config(DOMNode* node, CORBACommons::CorbaConfig& corba_config,
      const char* xml_namespace) /*throw (eh::Exception, Exception)*/;

    static
    void
    read_client_config(DOMNode* node,
      CORBACommons::CorbaClientConfig& corba_config,
      const char* xml_namespace)
      /*throw (eh::Exception, Exception)*/;

    static
    void
    read_corba_ref(DOMNode* node,
      CORBACommons::CorbaObjectRef& corba_object_ref,
      std::string& object_name, const char* xml_namespace)
      /*throw (eh::Exception, Exception)*/;

    static
    void
    read_corba_connection(DOMNode* node,
      CORBACommons::CorbaObjectRef& corba_object_ref,
      std::string& object_name, const char* xml_namespace)
      /*throw (eh::Exception, Exception)*/;

  protected:
    static
    void
    read_endpoint(DOMNode* node,
      CORBACommons::EndpointConfig& endpoint_config,
      const char* xml_namespace)
      /*throw (eh::Exception, Exception)*/;

    static
    void
    read_secure_params(DOMNode* node,
      CORBACommons::SecureConnectionConfig& secure_connection_config)
      /*throw (eh::Exception, Exception)*/;
  };
}

#endif
