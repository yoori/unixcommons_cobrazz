#ifndef USERVER_STATISTICS_TIMESTATISTICSPROVIDER_HPP
#define USERVER_STATISTICS_TIMESTATISTICSPROVIDER_HPP

// STD
#include <array>
#include <atomic>
#include <chrono>
#include <vector>

// THIS
#include <eh/Exception.hpp>
#include <Generics/Function.hpp>
#include <UServerUtils/Grpc/Statistics/StatisticsProvider.hpp>

namespace UServerUtils::Statistics
{

template<class T>
concept TimeStatisticsEnum = std::is_enum_v<T> && requires(T t)
{
  T::Max;
};

template<class T, class E>
concept EnumToStringConverter =
  std::is_object_v<T> &&
  std::is_default_constructible_v<T> &&
  std::is_invocable_r_v<std::map<E, std::string>, T>;

template<
  TimeStatisticsEnum Enum,
  EnumToStringConverter<Enum> Converter,
  const std::size_t number_intervals,
  const std::size_t time_interval_ms>
class TimeStatisticsProvider final : public StatisticsProvider
{
public:
  using CounterArray = std::array<std::atomic<std::uint64_t>, number_intervals>;
  using Statistics = std::vector<CounterArray>;
  using StatisticsPtr = std::shared_ptr<Statistics>;
  using LabelView = userver::utils::statistics::LabelView;
  using Label = userver::utils::statistics::Label;
  using EnumLabels = std::vector<Label>;
  using TimeLabels = std::vector<Label>;

  class Measure final : private Generics::Uncopyable
  {
  private:
    using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

  public:
    explicit Measure(
      const StatisticsPtr& statistics,
      const Enum id) noexcept
      : statistics_(statistics),
        id_(id),
        begin_time_point_(std::chrono::high_resolution_clock::now())
    {
    }

    ~Measure()
    {
      if (statistics_->empty())
        return;

      const auto end_time_point = std::chrono::high_resolution_clock::now();
      const std::size_t elapsed_time_ms = std::chrono::duration<double, std::milli>(
        end_time_point - begin_time_point_).count();

      std::size_t index = elapsed_time_ms / time_interval_ms;
      if (index >= number_intervals)
      {
        index = number_intervals - 1;
      }
      (*statistics_)[static_cast<std::size_t>(id_)][index].fetch_add(1, std::memory_order_relaxed);
    }

  private:
    const StatisticsPtr statistics_;

    const Enum id_;

    const TimePoint begin_time_point_;
  };

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  TimeStatisticsProvider()
   : statistics_(std::make_shared<Statistics>(
       static_cast<std::size_t>(Enum::Max)))
  {
    const auto names = Converter{}();

    if (names.size() != static_cast<std::size_t>(Enum::Max))
    {
      std::ostringstream stream;
      stream << FNS
             << "Not correct size of Names. "
             << "Expected number = "
             << static_cast<std::size_t>(Enum::Max)
             << ", received number = "
             << names.size();
      throw Exception(stream.str());
    }

    if (static_cast<std::size_t>(Enum::Max) == 0)
    {
      return;
    }

    for (auto& array : *statistics_)
    {
      for (auto& v : array)
      {
        std::atomic_init(&v, std::uint64_t(0));
      }
    }

    enum_labels_.reserve(names.size());
    int expected_value = 0;
    for (const auto& [enum_id, name] : names)
    {
      if (expected_value++ != static_cast<int>(enum_id))
      {
        std::ostringstream stream;
        stream << FNS
               << "Not correct TimeStatisticsEnum";
        throw Exception(stream.str());
      }

      enum_labels_.emplace_back("name", name);
    }

    time_labels_.reserve(number_intervals);
    for (std::size_t i = 1; i < number_intervals; i += 1)
    {
      std::ostringstream stream;
      stream << "["
             << time_interval_ms * (i - 1)
             << "ms; "
             << time_interval_ms * i
             << "ms)";
      time_labels_.emplace_back("time", stream.str());
    }

    std::ostringstream stream;
    stream << "["
           << time_interval_ms * (number_intervals - 1)
           << "ms; "
           << "infinity)";
    time_labels_.emplace_back("time", stream.str());
  }

  Measure make_measure(const Enum id) noexcept
  {
    return Measure(statistics_, id);
  }

  std::string name() override
  {
    return "time_statistic";
  }

private:
  void write(Writer& writer) override
  {
    auto& statistics = *statistics_;
    const auto size_statistics = statistics.size();
    const auto array_size = number_intervals;

    for (std::size_t i = 0; i < size_statistics; ++i)
    {
      for (std::size_t k = 0; k < array_size; ++k)
      {
        writer.ValueWithLabels(
          statistics[i][k].exchange(0, std::memory_order_relaxed),
          {
            LabelView(enum_labels_[i]),
            LabelView(time_labels_[k])
          });
      }
    }
  }

private:
  StatisticsPtr statistics_;

  EnumLabels enum_labels_;

  TimeLabels time_labels_;
};

template<
  TimeStatisticsEnum Enum,
  EnumToStringConverter<Enum> Converter,
  const std::size_t number_intervals,
  const std::size_t time_interval_ms>
auto get_time_statistics_provider()
{
  using Provider = TimeStatisticsProvider<
    Enum,
    Converter,
    number_intervals,
    time_interval_ms>;

  static std::shared_ptr<Provider> ptr(new Provider);
  return ptr;
}

} // namespace UServerUtils::Statistics

#endif //USERVER_STATISTICS_TIMESTATISTICSPROVIDER_HPP