#ifndef ___CompositeMetricsProvider__H
#define ___CompositeMetricsProvider__H
#include <map>
#include <set>
#include "MetricsProvider.hpp"
#include "ReferenceCounting/SmartPtr.hpp"
class CompositeMetricsProvider : public MetricsProvider
{
    std::map<std::string,Value> container;
    std::set<ReferenceCounting::SmartPtr<MetricsProvider> > providers;
public:
  void
  add_provider(MetricsProvider* p);


  MetricArray get_values();
  std::map<std::string,std::string> getStringValues();

};

#endif

