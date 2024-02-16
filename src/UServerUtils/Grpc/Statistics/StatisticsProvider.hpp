#ifndef USERVER_STATISTICS_STATISTICSPROVIDER_HPP
#define USERVER_STATISTICS_STATISTICSPROVIDER_HPP

// STD
#include <memory>

// USERVER
#include <userver/utils/statistics/writer.hpp>

// THIS
#include <Generics/Uncopyable.hpp>

namespace UServerUtils::Statistics
{

using Writer = userver::utils::statistics::Writer;

class StatisticsProvider : private Generics::Uncopyable
{
public:
  StatisticsProvider() = default;

  virtual ~StatisticsProvider() = default;

  virtual void write(Writer& writer) = 0;

  virtual std::string name() = 0;
};

using StatisticsProviderPtr = std::shared_ptr<StatisticsProvider>;

} // namespace UServerUtils::Statistics

#endif // USERVER_STATISTICS_STATISTICSPROVIDER_HPP
