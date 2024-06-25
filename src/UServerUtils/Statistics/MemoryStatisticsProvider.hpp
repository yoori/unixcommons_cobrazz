#ifndef USERVER_STATISTICS_MEMORYSTATISTICSPROVIDER_HPP
#define USERVER_STATISTICS_MEMORYSTATISTICSPROVIDER_HPP

// USERVER
#include <userver/utils/statistics/labels.hpp>

// THIS
#include <eh/Exception.hpp>
#include <UServerUtils/Statistics/StatisticsProvider.hpp>

namespace UServerUtils::Statistics
{

class MemoryStatisticsProvider final : public StatisticsProvider
{
private:
  using Label = userver::utils::statistics::Label;
  using LabelView = userver::utils::statistics::LabelView;

  struct MallocInfo final
  {
    std::uint64_t total = 0;
    std::uint64_t in_use = 0;
  };

  struct JemallocInfo final
  {
    std::uint64_t allocated = 0;
    std::uint64_t active = 0;
    std::uint64_t metadata = 0;
    std::uint64_t resident = 0;
    std::uint64_t mapped = 0;
    std::uint64_t retained = 0;
  };

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  MemoryStatisticsProvider() = default;

  ~MemoryStatisticsProvider() override = default;

  void write(Writer& writer) override;

  std::string name() override;

private:
  MallocInfo get_malloc_info();

  JemallocInfo get_jemalloc_info();
};

} // namespace UServerUtils::Statistics

#endif //USERVER_STATISTICS_MEMORYSTATISTICSPROVIDER_HPP
