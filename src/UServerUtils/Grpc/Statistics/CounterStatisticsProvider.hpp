#ifndef USERVER_STATISTICS_COUNTERSTATISTICSPROVIDER_HPP
#define USERVER_STATISTICS_COUNTERSTATISTICSPROVIDER_HPP

// STD
#include <map>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

// USERVER
#include <userver/utils/statistics/labels.hpp>

// THIS
#include <eh/Exception.hpp>
#include <Generics/Function.hpp>
#include <UServerUtils/Grpc/Statistics/Concept.hpp>
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

template<class T, class E>
concept EnumConverterConcept =
  std::is_object_v<T> &&
  std::is_default_constructible_v<T> &&
  std::is_invocable_r_v<
    std::map<E, std::pair<UServerUtils::Statistics::CounterType, std::string>>,
    T>;

} // namespace Internal::Counter

template<
  EnumConcept Enum,
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
  CounterStatisticsProvider(const std::string& provider_name = "counter")
    : provider_name_(provider_name),
      statistics_(static_cast<std::size_t>(Enum::Max))
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

  template<NumericConcept T>
  void add(const Enum id, const T t) noexcept
  {
    auto& data = statistics_[static_cast<std::size_t>(id)];
    switch (data.type)
    {
    case CounterType::UInt:
      data.uint_value.fetch_add(static_cast<std::uint64_t>(t), std::memory_order_relaxed);
      break;
    case CounterType::Int:
      data.int_value.fetch_add(static_cast<std::int64_t>(t), std::memory_order_relaxed);
      break;
    case CounterType::Double:
      data.double_value.fetch_add(static_cast<double>(t), std::memory_order_relaxed);
      break;
    case CounterType::Bool:
      data.bool_value.store(static_cast<bool>(t), std::memory_order_relaxed);
      break;
    }
  }

  template<NumericConcept T>
  void set(const Enum id, const T t) noexcept
  {
    auto& data = statistics_[static_cast<std::size_t>(id)];
    switch (data.type)
    {
    case CounterType::UInt:
      data.uint_value.store(static_cast<std::uint64_t>(t), std::memory_order_relaxed);
      break;
    case CounterType::Int:
      data.int_value.store(static_cast<std::int64_t>(t), std::memory_order_relaxed);
      break;
    case CounterType::Double:
      data.double_value.store(static_cast<double>(t), std::memory_order_relaxed);
      break;
    case CounterType::Bool:
      data.bool_value.store(static_cast<bool>(t), std::memory_order_relaxed);
      break;
    }
  }

  std::string name() override
  {
    return provider_name_;
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
          statistic.uint_value.load(std::memory_order_relaxed),
          {LabelView(statistic.label)});
        break;
      case CounterType::Int:
        writer.ValueWithLabels(
          statistic.int_value.load(std::memory_order_relaxed),
          {LabelView(statistic.label)});
        break;
      case CounterType::Double:
        writer.ValueWithLabels(
          statistic.double_value.load(std::memory_order_relaxed),
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
  const std::string provider_name_;

  Statistics statistics_;
};

template<
  EnumConcept Enum,
  Internal::Counter::EnumConverterConcept<Enum> Converter>
auto& get_counter_statistics_provider(const std::string& provider_name = "counter")
{
  using Provider = CounterStatisticsProvider<Enum, Converter>;
  static std::shared_ptr<Provider> ptr(new Provider(provider_name));
  return ptr;
}

} // namespace UServerUtils::Statistics

#endif //USERVER_STATISTICS_COUNTERSTATISTICSPROVIDER_HPP