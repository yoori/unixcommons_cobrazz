#pragma once

#include <string>
#include <thread>
#include <ReferenceCounting/SmartPtr.hpp>
#include "Generics/MetricsProvider.hpp"

#include <Generics/ActiveObject.hpp>
#include <Generics/MetricsProvider.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>

namespace UServerUtils
{
  const extern std::string config_z_yaml;

  class MetricsHTTPProvider:
    public Generics::SimpleActiveObject,
    public ReferenceCounting::AtomicImpl
  {
  public:
    MetricsHTTPProvider(
      Generics::MetricsProvider* metrics_provider,
      unsigned int listen_port,
      std::string_view uri);

    ~MetricsHTTPProvider();

  private:
    static void* worker(MetricsHTTPProvider* _this);

    void
    activate_object_() override;

    void
    wait_object_() override;

  private:
    const int listen_port_;
    const std::string uri_;
    std::thread thread_;

  public:
    static ReferenceCounting::SmartPtr<Generics::MetricsProvider> container;
  };

  typedef ReferenceCounting::SmartPtr<MetricsHTTPProvider> MetricsHTTPProvider_var;
}
