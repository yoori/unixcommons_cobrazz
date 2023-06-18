#pragma once

#include <string>
#include <thread>
#include <ReferenceCounting/SmartPtr.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/MetricsProvider.hpp>
#include <Sync/Condition.hpp>

namespace UServerUtils
{
  const extern std::string config_z_yaml;

  class MetricsHTTPProvider: public Generics::SimpleActiveObject,
    public ReferenceCounting::AtomicImpl
  {
  public:
    MetricsHTTPProvider(Generics::MetricsProvider* mProv, unsigned int _listen_port, std::string _uri);

    ~MetricsHTTPProvider();

    // ActiveObject interface
    void
    activate_object() override;

    void
    deactivate_object() override;

    void
    wait_object() override;

  private:

    static void* worker(MetricsHTTPProvider* _this);

  private:
    const int listen_port_;
    const std::string uri_;
    std::thread thread_;
    Sync::Condition cond_;


  public:
    // TO FIX: why it is static ? MetricsHTTPProvider can be used only once in app ?
    static ReferenceCounting::SmartPtr<Generics::MetricsProvider> container;
    
  };

  typedef ReferenceCounting::SmartPtr<MetricsHTTPProvider> MetricsHTTPProvider_var;
}
