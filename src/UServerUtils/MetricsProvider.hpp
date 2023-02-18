#ifndef __METRICS_PROVIDER__H
#define __METRICS_PROVIDER__H
#include <vector>
#include <boost/variant.hpp>
#include "ReferenceCounting/AtomicImpl.hpp"
class MetricsProvider : public ReferenceCounting::AtomicImpl
{
public:
  typedef boost::variant<double, long, std::string> Value;
  typedef std::vector<std::pair<std::string, Value> > MetricArray;

  virtual MetricArray get_values() = 0;
  virtual void add_value(double v)=0;
  virtual void add_value(long v)=0;
  virtual void add_value(const std::string& v)=0;
};


#endif