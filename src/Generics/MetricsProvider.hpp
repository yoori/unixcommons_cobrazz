#pragma once

#include <string>
#include <vector>

#include <boost/variant.hpp>

#include <ReferenceCounting/AtomicImpl.hpp>

namespace Generics
{
  class MetricsProvider : public ReferenceCounting::AtomicImpl
  {
  public:
    typedef boost::variant<double, long, std::string> Value;
    typedef std::vector<std::pair<std::string, Value> > MetricArray;

    virtual MetricArray
    get_values() = 0;

  protected:
    virtual
    ~MetricsProvider() noexcept = default;
  };
}
