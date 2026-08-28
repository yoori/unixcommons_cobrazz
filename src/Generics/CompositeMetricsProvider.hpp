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
    void add_provider(MetricsProvider* provider);

    MetricArray get_values() override;

    std::map<std::string, std::string>
    getStringValues();

  private:
    using ProviderSet = std::set<ReferenceCounting::SmartPtr<MetricsProvider> >;

    ProviderSet providers_;
    std::mutex lock_;
  };

  using CompositeMetricsProvider_var = ReferenceCounting::SmartPtr<CompositeMetricsProvider>;
}
