/**
 * @author Pavel Gubin <pgubin@ipmce.ru>
 */
#ifndef APACHE_PROXY_INJECTOR_HPP
#define APACHE_PROXY_INJECTOR_HPP

#include <ReferenceCounting/ReferenceCounting.hpp>

#include <Apache/Module.hpp>


class ProxyInjectorModule :
  public Apache::InsertFilterHook<ProxyInjectorModule>,
  public ReferenceCounting::AtomicImpl
{
public:
  typedef ReferenceCounting::QualPtr<ProxyInjectorModule>
    ProxyInjectorModule_var;
  static ProxyInjectorModule_var instance;

  class InjectorFilter : public Apache::RequestOutputFilter
  {
  public:
    InjectorFilter(request_rec* r) throw ();
    virtual apr_status_t
    filter(ap_filter_t* f, apr_bucket_brigade* bb) throw ();

  private:
    apr_bucket_brigade* bb_;
  };

public:
  ProxyInjectorModule() /*throw (eh::Exception)*/;

  virtual void
  insert_filter(request_rec* r) throw ();

protected:
  virtual
  ~ProxyInjectorModule() throw ();

private:
  int test_;
};

#endif // _PROXY_INJECTOR_HPP_
