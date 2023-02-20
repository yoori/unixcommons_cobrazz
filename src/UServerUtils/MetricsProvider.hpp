#ifndef __METRICS_PROVIDER__H
#define __METRICS_PROVIDER__H
#include <vector>
#include <boost/variant.hpp>
#include "ReferenceCounting/AtomicImpl.hpp"
#include <string_view>
namespace Generics{
class MetricsProvider : public ReferenceCounting::AtomicImpl
{
public:
  typedef boost::variant<double, long, std::string> Value;
  typedef std::vector<std::pair<std::string, Value> > MetricArray;

  virtual MetricArray get_values() = 0;
  virtual void add_value(std::string_view n,double v)=0;
  virtual void add_value(std::string_view n,long v)=0;
  virtual void add_value(std::string_view n,const std::string& v)=0;
};

}
#endif