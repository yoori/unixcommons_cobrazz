#ifndef ___CompositeMetricsProvider__H
#define ___CompositeMetricsProvider__H
#include "MetricsProvider.hpp"
class CompositeMetricsProvider : public MetricsProvider
{
    std::map<std::string,Value> container;
    std::set<REF_getter<MetricsProvider> > providers;
public:
  void
  add_provider(MetricsProvider* p);


  MetricArray get_values();
  std::map<std::string,std::string> getStringValues();

};

#endif

