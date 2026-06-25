#pragma once

#include <map>
#include <mutex>
#include <set>

#include <ReferenceCounting/SmartPtr.hpp>

#include "MetricsProvider.hpp"

namespace Generics
{
  class CompositeMetricsProvider : public MetricsProvider
  {
  public:
    void
    add_provider(MetricsProvider* provider);

    MetricArray
    get_values() override;

    std::map<std::string, std::string>
    getStringValues();

  private:
    typedef std::set<ReferenceCounting::SmartPtr<MetricsProvider> >
      ProviderSet;

    ProviderSet providers_;
    std::mutex lock_;
  };

  typedef ReferenceCounting::SmartPtr<CompositeMetricsProvider>
    CompositeMetricsProvider_var;
}
