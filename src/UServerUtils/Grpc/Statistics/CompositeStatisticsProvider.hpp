#ifndef USERVER_STATISTICS_COMPOSITESTATISTICSPROVIDER_HPP
#define USERVER_STATISTICS_COMPOSITESTATISTICSPROVIDER_HPP

// STD
#include <deque>
#include <shared_mutex>

// THIS
#include <Logger/Logger.hpp>
#include <UServerUtils/Grpc/Statistics/Concept.hpp>
#include <UServerUtils/Grpc/Statistics/StatisticsProvider.hpp>

namespace UServerUtils::Statistics
{

class CompositeStatisticsProvider : public StatisticsProvider
{
public:
  CompositeStatisticsProvider() = default;

  ~CompositeStatisticsProvider() override = default;

  virtual void add(const StatisticsProviderPtr& statistics_provider) = 0;
};

template<SharedMutexConcept SharedMutex = std::shared_mutex, template<class> class Container = std::deque>
class CompositeStatisticsProviderImpl final : public CompositeStatisticsProvider
{
private:
  using Providers = Container<StatisticsProviderPtr>;
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;

public:
  CompositeStatisticsProviderImpl(Logger* logger)
    : logger_(ReferenceCounting::add_ref(logger))
  {
  }

  ~CompositeStatisticsProviderImpl() override = default;

  void add(const StatisticsProviderPtr& statistics_provider) override
  {
    std::unique_lock lock(mutex_);
    providers_.emplace_back(statistics_provider);
  }

private:
  void write(Writer& writer) override
  {
    static const char aspect[] = "COMPOSITE_STATISTICS_PROVIDER";

    std::shared_lock lock(mutex_);
    for (auto& provider : providers_)
    {
      auto child_writer = writer[provider->name()];
      try
      {
        provider->write(child_writer);
      }
      catch (const std::exception& exc)
      {
        std::ostringstream stream;
        stream << FNS
               << exc.what();
        logger_->error(stream.str(), aspect);
      }
      catch (...)
      {
        std::ostringstream stream;
        stream << FNS
               << "Unknown error";
        logger_->error(stream.str(), aspect);
      }
    }
  }

  std::string name() override
  {
    return "composite_statistics";
  }

private:
  const Logger_var logger_;

  SharedMutex mutex_;

  Providers providers_;
};

using CompositeStatisticsProviderPtr = std::shared_ptr<CompositeStatisticsProvider>;

} // namespace UServerUtils::Statistics

#endif //USERVER_STATISTICS_COMPOSITESTATISTICSPROVIDER_HPP