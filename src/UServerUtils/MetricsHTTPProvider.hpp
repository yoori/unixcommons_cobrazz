#ifndef ___METRICS___H
#define ___METRICS___H

#include <string>
#include <thread>
#include <ReferenceCounting/SmartPtr.hpp>

#ifdef __MACH__
namespace Generics {
class ActiveObject
{
public:
    virtual
    void
    activate_object()    =0;
    virtual
    void
    deactivate_object() =0;
    virtual
    void
    wait_object() =0;
    virtual
    bool
    active() =0;
    virtual ~ActiveObject(){}
};

}
#elif defined(__linux__)


#include <Generics/ActiveObject.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#else
#error "! linux && ! macos":
#endif

namespace UServerUtils
{
  class MetricsHTTPProviderImpl;

  class MetricsHTTPProvider: public Generics::ActiveObject
#ifdef __linux__
    , public ReferenceCounting::AtomicImpl
#endif
  {
    friend class MetricsHTTPProviderImpl;

  public:
    MetricsHTTPProvider(unsigned int _listen_port, std::string _uri);

    ~MetricsHTTPProvider();

    // собираем значения от приложения
    void
    set_value(std::string_view key, std::string_view value);

    void
    set_value(std::string_view key, unsigned long value);

    void
    add_value(std::string_view key, unsigned long value);

    // override ActiveObject methods
    void
    activate_object() override; // поднять сервис в отдельном потоке

    void
    deactivate_object() override; // поднять сервис в отдельном потоке

    void
    wait_object() override; // дождаться окончания остановки (типа join потока)

    bool
    active() override;

  private:
    ReferenceCounting::FixedPtr<MetricsHTTPProviderImpl> impl_;

  private:
    static const std::string config;
  };

  typedef ReferenceCounting::SmartPtr<MetricsHTTPProvider> MetricsHTTPProvider_var;
}

#endif
