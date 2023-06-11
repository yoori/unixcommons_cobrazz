#ifndef __METRICS_PROVIDER__H
#define __METRICS_PROVIDER__H

#include <vector>
#include <boost/variant.hpp>
#include <string_view>
#include <map>

#include <ReferenceCounting/AtomicImpl.hpp>

namespace Generics
{
  class MetricsProvider : public ReferenceCounting::AtomicImpl
  {
  public:
    typedef boost::variant<double, long, std::string> Value;
    typedef std::vector<std::pair<std::string, Value> > MetricArray;

    virtual MetricArray get_values() = 0;
    virtual void add_value(std::string_view n,double v)=0;
    virtual void add_value(std::string_view n,long v)=0;
    virtual void add_value(std::string_view n,const std::string& v)=0;
    virtual void add_value_prometheus(const std::string & parameter, const std::map<std::string, std::string>& par2, double value)=0;
    virtual void set_value_prometheus(const std::string & parameter, const std::map<std::string, std::string>& par2, double value)=0;

  };
}

#endif
