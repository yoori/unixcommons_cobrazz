#pragma once

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
    InjectorFilter(request_rec* r) noexcept;
    virtual apr_status_t
    filter(ap_filter_t* f, apr_bucket_brigade* bb) noexcept;

  private:
    apr_bucket_brigade* bb_;
  };

public:
  ProxyInjectorModule() /*throw (eh::Exception)*/;

  virtual void
  insert_filter(request_rec* r) noexcept;

protected:
  virtual
  ~ProxyInjectorModule() noexcept;

private:
  int test_;
};
