#pragma once

#include <string>
#include <thread>
#include <ReferenceCounting/SmartPtr.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <Generics/ActiveObject.hpp>
#include <Generics/MetricsProvider.hpp>

namespace UServerUtils
{
  const extern std::string config_z_yaml;

  class MetricsHTTPProvider: public Generics::ActiveObject,
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

    bool
    active() override; // is started ?

  private:
    // TO FIX: remove, AtomicImpl don't allow copying
    MetricsHTTPProvider(const MetricsHTTPProvider&) = delete; // protect from usage

    MetricsHTTPProvider& operator=(const MetricsHTTPProvider&) = delete; // protect from usage

    static void* worker(MetricsHTTPProvider* _this);

  private:
    const int listen_port_;
    const std::string uri_;
    std::thread thread_;

    // TO FIX: use SimpleActiveObject instead stopped_, state_ (and implement activate_object_, deactivate_object_)
    bool stopped_ = false;
    volatile sig_atomic_t state_;

  public:
    // TO FIX: why it is static ? MetricsHTTPProvider can be used only once in app ?
    static ReferenceCounting::SmartPtr<Generics::MetricsProvider> container;
  };

  typedef ReferenceCounting::SmartPtr<MetricsHTTPProvider> MetricsHTTPProvider_var;
}
