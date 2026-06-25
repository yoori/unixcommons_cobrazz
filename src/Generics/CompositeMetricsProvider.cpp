#include "CompositeMetricsProvider.hpp"

namespace Generics
{
  namespace
  {
    class ToStringVisitor : public boost::static_visitor<>
    {
    public:
      std::string str;

      void
      operator()(long value)
      {
        str = std::to_string(value);
      }

      void
      operator()(double value)
      {
        str = std::to_string(value);
      }

      void
      operator()(const std::string& value)
      {
        str = value;
      }
    };
  }

  void
  CompositeMetricsProvider::add_provider(MetricsProvider* provider)
  {
    std::lock_guard<std::mutex> guard(lock_);
    providers_.insert(ReferenceCounting::add_ref(provider));
  }

  MetricsProvider::MetricArray
  CompositeMetricsProvider::get_values()
  {
    ProviderSet providers;
    {
      std::lock_guard<std::mutex> guard(lock_);
      providers = providers_;
    }

    MetricArray result;
    for(const auto& provider : providers)
    {
      MetricArray values = provider->get_values();
      result.insert(result.end(), values.begin(), values.end());
    }
    return result;
  }

  std::map<std::string, std::string>
  CompositeMetricsProvider::getStringValues()
  {
    std::map<std::string, std::string> result;
    MetricArray values = get_values();
    for(const auto& value : values)
    {
      ToStringVisitor visitor;
      boost::apply_visitor(visitor, value.second);
      result[value.first] = visitor.str;
    }
    return result;
  }
}
