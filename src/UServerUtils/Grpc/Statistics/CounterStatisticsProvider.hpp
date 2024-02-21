#ifndef USERVER_STATISTICS_COUNTERSTATISTICSPROVIDER_HPP
#define USERVER_STATISTICS_COUNTERSTATISTICSPROVIDER_HPP

// STD
#include <map>
#include <string>
#include <variant>
#include <vector>

// USERVER
#include <userver/utils/statistics/labels.hpp>

// THIS
#include <eh/Exception.hpp>
#include <Generics/Function.hpp>
#include <UServerUtils/Grpc/Statistics/StatisticsProvider.hpp>

namespace UServerUtils::Statistics
{

enum class CounterType
{
  UInt,
  Int,
  Double,
  Bool
};

namespace Internal::Counter
{

template<class T>
concept EnumConcept = std::is_enum_v<T> && requires(T t)
{
  T::Max;
};

template<class T, class E>
concept EnumConverterConcept =
  std::is_object_v<T> &&
  std::is_default_constructible_v<T> &&
  std::is_invocable_r_v<
    std::map<E, std::pair<UServerUtils::Statistics::CounterType, std::string>>,
    T>;

template<typename T>
concept Numeric = std::is_arithmetic_v<T>;

} // namespace Internal::Counter

template<
  Internal::Counter::EnumConcept Enum,
  Internal::Counter::EnumConverterConcept<Enum> Converter>
class CounterStatisticsProvider final : public StatisticsProvider
{
private:
  using Label = userver::utils::statistics::Label;
  using LabelView = userver::utils::statistics::LabelView;

  struct Data final
  {
    Data() = default;
    ~Data() = default;

    CounterType type = UServerUtils::Statistics::CounterType::UInt;
    Label label;
    std::atomic<std::uint64_t> uint_value{0};
    std::atomic<std::int64_t> int_value{0};
    std::atomic<double> double_value{0.0};
    std::atomic<bool> bool_value{false};
  };
  using Statistics = std::vector<Data>;

public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  CounterStatisticsProvider()
    : statistics_(static_cast<std::size_t>(Enum::Max))
  {
    const auto names_with_typed = Converter{}();
    if (names_with_typed.size() != static_cast<std::size_t>(Enum::Max))
    {
      std::ostringstream stream;
      stream << FNS
             << "Not correct size of Names. "
             << "Expected number = "
             << static_cast<std::size_t>(Enum::Max)
             << ", received number = "
             << names_with_typed.size();
      throw Exception(stream.str());
    }

    if (static_cast<std::size_t>(Enum::Max) == 0)
    {
      return;
    }

    int expected_value = 0;
    for (const auto& [enum_id, data] : names_with_typed)
    {
      if (expected_value++ != static_cast<int>(enum_id))
      {
        std::ostringstream stream;
        stream << FNS
               << "Not correct Enum";
        throw Exception(stream.str());
      }

      statistics_[static_cast<int>(enum_id)].type = data.first;
      statistics_[static_cast<int>(enum_id)].label = Label("name", data.second);
    }
  }

  ~CounterStatisticsProvider() override = default;

  template<Internal::Counter::Numeric T>
  void add(const Enum id, const T t) noexcept
  {
    auto& data = statistics_[static_cast<std::size_t>(id)];
    switch (data.type)
    {
    case CounterType::UInt:
      data.uint_value.fetch_add(
        static_cast<std::uint64_t>(t),
        std::memory_order_relaxed);
      break;
    case CounterType::Int:
      data.int_value.fetch_add(
        static_cast<std::int64_t>(t),
        std::memory_order_relaxed);
      break;
    case CounterType::Double:
      data.double_value.fetch_add(
        static_cast<double>(t),
        std::memory_order_relaxed);
      break;
    case CounterType::Bool:
      data.bool_value.store(
        static_cast<bool>(t),
        std::memory_order_relaxed);
      break;
    }
  }

  template<Internal::Counter::Numeric T>
  void set(const Enum id, const T t) noexcept
  {
    auto& data = statistics_[static_cast<std::size_t>(id)];
    switch (data.type)
    {
    case CounterType::UInt:
      data.uint_value.store(
        static_cast<std::uint64_t>(t),
        std::memory_order_relaxed);
      break;
    case CounterType::Int:
      data.int_value.store(
        static_cast<std::int64_t>(t),
        std::memory_order_relaxed);
      break;
    case CounterType::Double:
      data.double_value.store(
        static_cast<double>(t),
        std::memory_order_relaxed);
      break;
    case CounterType::Bool:
      data.bool_value.store(
        static_cast<bool>(t),
        std::memory_order_relaxed);
      break;
    }
  }

  std::string name() override
  {
    return "counter";
  }

private:
  void write(Writer& writer) override
  {
    for (auto& statistic : statistics_)
    {
      const auto type = statistic.type;
      switch (type)
      {
      case CounterType::UInt:
        writer.ValueWithLabels(
          statistic.uint_value.exchange(0, std::memory_order_relaxed),
          {LabelView(statistic.label)});
        break;
      case CounterType::Int:
        writer.ValueWithLabels(
          statistic.int_value.exchange(0, std::memory_order_relaxed),
          {LabelView(statistic.label)});
        break;
      case CounterType::Double:
        writer.ValueWithLabels(
          statistic.double_value.exchange(0, std::memory_order_relaxed),
          {LabelView(statistic.label)});
        break;
      case CounterType::Bool:
        writer.ValueWithLabels(
          statistic.bool_value.load(std::memory_order_relaxed),
          {LabelView(statistic.label)});
        statistic.double_value.store(false, std::memory_order_relaxed);
        break;
      }
    }
  }

private:
  Statistics statistics_;
};

template<
  Internal::Counter::EnumConcept Enum,
  Internal::Counter::EnumConverterConcept<Enum> Converter>
auto get_counter_statistics_provider()
{
  using Provider = CounterStatisticsProvider<Enum, Converter>;
  static std::shared_ptr<Provider> ptr(new Provider);
  return ptr;
}

} // namespace UServerUtils::Statistics

#endif //USERVER_STATISTICS_COUNTERSTATISTICSPROVIDER_HPP