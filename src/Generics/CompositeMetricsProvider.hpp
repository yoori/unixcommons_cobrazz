#ifndef ___CompositeMetricsProvider__H
#define ___CompositeMetricsProvider__H
#include <map>
#include <set>
#include <mutex>
#include "MetricsProvider.hpp"
#include "ReferenceCounting/SmartPtr.hpp"
namespace Generics
{
class CompositeMetricsProvider : public MetricsProvider
{
    std::map<std::string,Value> container;
    std::set<ReferenceCounting::SmartPtr<MetricsProvider> > providers;
    std::mutex mx;
public:
  void
  add_provider(MetricsProvider* p);


  MetricArray get_values();
  std::map<std::string,std::string> getStringValues();
  
  void add_value(std::string_view n,double v)
  {
    std::lock_guard<std::mutex> g(mx);
    container[std::string(n)]=v;
  }
  void add_value(std::string_view n,long v)
  {
    std::lock_guard<std::mutex> g(mx);
    container[std::string(n)]=v;
  }
  void add_value(std::string_view n,const std::string& v)
  {
    std::lock_guard<std::mutex> g(mx);
    container[std::string(n)]=v;
  }


};
}
#endif

