#ifndef ___METRICS___H
#define ___METRICS___H

#include <string>
#include <thread>
#include <ReferenceCounting/SmartPtr.hpp>
#include "Generics/MetricsProvider.hpp"
using namespace Generics;


#include <Generics/ActiveObject.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>

namespace UServerUtils
{
  const extern std::string config_z_yaml;


  class MetricsHTTPProvider: public Generics::ActiveObject
    , public ReferenceCounting::AtomicImpl
  {
  public:
    MetricsHTTPProvider(MetricsProvider * mProv,unsigned int _listen_port, std::string_view _uri);

    ~MetricsHTTPProvider();


    // определяем интерфейс ActiveObject
    void
    activate_object() override; // поднять сервис в отдельном потоке

    void
    deactivate_object() override; // начать остановку сервиса

    void
    wait_object() override; // дождаться окончания остановки (типа join потока)

    bool
    active() override; // is started ?

  private:
    MetricsHTTPProvider(const MetricsHTTPProvider&) = delete; // protect from usage

    MetricsHTTPProvider& operator=(const MetricsHTTPProvider&) = delete; // protect from usage

  private:
    int listen_port_;
    std::string_view uri_;
    std::thread thread_;
    static void* worker(MetricsHTTPProvider* _this);
    
    bool stopped_=false;

    bool stop_ = false;
    volatile sig_atomic_t state_;

public:
    static ReferenceCounting::SmartPtr<MetricsProvider> container;

  };

  typedef ReferenceCounting::SmartPtr<MetricsHTTPProvider> MetricsHTTPProvider_var;
}

#endif
