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
    using Value = boost::variant<double, long, std::string>;
    using MetricArray = std::vector<std::pair<std::string, Value> >;

    virtual MetricArray get_values() = 0;

  protected:
    virtual ~MetricsProvider() noexcept = default;
  };
}
