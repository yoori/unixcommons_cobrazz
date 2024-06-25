#ifndef USERVER_STATISTICS_COMMONSTATISTICSPROVIDER_HPP
#define USERVER_STATISTICS_COMMONSTATISTICSPROVIDER_HPP

// STD
#include <map>
#include <string>
#include <variant>
#include <vector>
#include <shared_mutex>
#include <sstream>

// USERVER
#include <userver/utils/statistics/labels.hpp>

// THIS
#include <eh/Exception.hpp>
#include <Generics/Function.hpp>
#include <UServerUtils/Statistics/Concept.hpp>
#include <UServerUtils/Statistics/StatisticsProvider.hpp>

namespace UServerUtils::Statistics
{

enum class CommonType
{
  UInt,
  Int,
  Bool,
  Double
};

namespace Internal::Common
{

template<class T, class E>
concept EnumConverterConcept =
  std::is_object_v<T> &&
  std::is_default_constructible_v<T> &&
  std::is_invocable_r_v<
    std::map<E, std::pair<UServerUtils::Statistics::CommonType, std::string>>,
    T>;

template<class T>
concept LabelConcept = std::is_same_v<T, std::string_view> || std::is_arithmetic_v<T>;

} // namespace Internal::Common

template<
  EnumConcept Enum,
  Internal::Common::EnumConverterConcept<Enum> Converter,
  SharedMutexConcept SharedMutex = std::shared_mutex,
  template<class, class> class Map = std::map>
class CommonStatisticsProvider final : public StatisticsProvider
{
private:
  using Label = userver::utils::statistics::Label;
  using LabelView = userver::utils::statistics::LabelView;

  struct Counter final
  {
    std::atomic<std::uint64_t> uint_value{0};
    std::atomic<std::int64_t> int_value{0};
    std::atomic<double> double_value{0.0};
    std::atomic<bool> bool_value{false};
  };

  using Key = std::variant<std::string_view, std::int64_t>;
  class Visitor final
  {
  public:
    Visitor() = default;

    ~Visitor() = default;

    std::string operator()(const std::string_view data)
    {
      return std::string(std::begin(data), std::end(data));
    }

    std::string operator()(const std::int64_t data)
    {
      return std::to_string(data);
    }
  };

  using Counters = Map<Key, Counter>;
  using Types = std::vector<CommonType>;
  using Names = std::vector<std::string>;
  using NameLabels = std::vector<std::string>;

  struct Statistic final
  {
    Statistic(Counters&& counters)
      : counters(std::move(counters))
    {
    }

    Counters counters;
    NameLabels name_labels;
    SharedMutex mutex;
  };
  using StatisticPtr = std::unique_ptr<Statistic>;
  using Statistics = std::vector<StatisticPtr>;

public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  CommonStatisticsProvider(
    const std::string& provider_name = "common",
    const std::optional<std::size_t> initial_size = {})
    : provider_name_(provider_name),
      initial_size_(initial_size)
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

      if (initial_size_.has_value())
      {
        if constexpr (std::is_constructible_v<Counters, std::size_t>)
        {
          statistics_.emplace_back(
            std::make_unique<Statistic>(Counters{*initial_size_}));
        }
        else
        {
          statistics_.emplace_back();
        }
      }
      else
      {
        statistics_.emplace_back(
          std::make_unique<Statistic>(Counters{}));
      }

      types_.emplace_back(data.first);
      names_.emplace_back(data.second);
    }
  }

  std::string name() override
  {
    return provider_name_;
  }

  template<Internal::Common::LabelConcept Label, NumericConcept N>
  void add(const Enum id, const Label& label, const N n)
  {
    const auto type = types_[static_cast<std::size_t>(id)];
    auto& statistic = statistics_[static_cast<std::size_t>(id)];
    auto& mutex = statistic->mutex;
    auto& counters = statistic->counters;
    auto& name_labels = statistic->name_labels;

    std::shared_lock shared_lock(mutex);
    auto it = counters.find(label);
    if (it != std::end(counters))
    {
      auto& counter = it->second;
      shared_lock.unlock();  // We only add and do not remove elements
      add(n, counter, type);
    }
    else
    {
      shared_lock.unlock();

      std::unique_lock unique_lock(mutex);
      it = counters.find(label);
      if (it == std::end(counters))
      {
        if constexpr (std::is_same_v<Label, std::string_view>)
        {
          name_labels.emplace_back(std::begin(label), std::end(label));
          Key key(std::string_view{name_labels.back()});
          it = counters.try_emplace(key).first;
        }
        else
        {
          it = counters.try_emplace(label).first;
        }
      }

      auto& counter = it->second;
      add(n, counter, type);
    }
  }

  template<Internal::Common::LabelConcept Label, NumericConcept N>
  void set(const Enum id, const Label& label, const N n)
  {
    const auto type = types_[static_cast<std::size_t>(id)];
    auto& statistic = statistics_[static_cast<std::size_t>(id)];
    auto& mutex = statistic->mutex;
    auto& counters = statistic->counters;
    auto& name_labels = statistic->name_labels;

    std::shared_lock shared_lock(mutex);
    auto it = counters.find(label);
    if (it != std::end(counters))
    {
      auto& counter = it->second;
      shared_lock.unlock();  // We only add and do not remove elements
      set(n, counter, type);
    }
    else
    {
      shared_lock.unlock();

      std::unique_lock unique_lock(mutex);
      it = counters.find(label);
      if (it == std::end(counters))
      {
        if constexpr (std::is_same_v<Label, std::string_view>)
        {
          name_labels.emplace_back(std::begin(label), std::end(label));
          Key key(std::string_view{name_labels.back()});
          it = counters.try_emplace(key).first;
        }
        else
        {
          it = counters.try_emplace(label).first;
        }
      }

      auto& counter = it->second;
      set(n, counter, type);
    }
  }

private:
  void write(Writer& writer) override
  {
    const auto size = statistics_.size();
    Visitor visitor;
    for (std::size_t i = 0; i < size; ++i)
    {
      auto& statistic = statistics_[i];
      auto& mutex = statistic->mutex;
      auto& counters = statistic->counters;
      const auto& name = names_[i];
      const auto type = types_[i];

      std::unique_lock lock(mutex);
      for (auto& [key, counter] : counters)
      {
        const std::string value = std::visit(visitor, key);
        switch (type)
        {
        case CommonType::UInt:
          writer.ValueWithLabels(
            counter.uint_value.load(std::memory_order_relaxed),
            {LabelView(name, value)});
          break;
        case CommonType::Int:
          writer.ValueWithLabels(
            counter.int_value.load(std::memory_order_relaxed),
            {LabelView(name, value)});
          break;
        case CommonType::Bool:
          writer.ValueWithLabels(
            counter.bool_value.load(std::memory_order_relaxed),
            {LabelView(name, value)});
          counter.bool_value.store(false, std::memory_order_relaxed);
          break;
        case CommonType::Double:
          writer.ValueWithLabels(
            counter.double_value.load(std::memory_order_relaxed),
            {LabelView(name, value)});
          break;
        }
      }
    }
  }

  template<NumericConcept N>
  void add(const N n, Counter& counter, const CommonType type)
  {
    switch (type)
    {
    case CommonType::UInt:
      counter.uint_value.fetch_add(static_cast<std::uint64_t>(n), std::memory_order_relaxed);
      break;
    case CommonType::Int:
      counter.int_value.fetch_add(static_cast<std::int64_t>(n), std::memory_order_relaxed);
      break;
    case CommonType::Bool:
      counter.bool_value.store(static_cast<bool>(n), std::memory_order_relaxed);
      break;
    case CommonType::Double:
      counter.double_value.fetch_add(static_cast<double>(n), std::memory_order_relaxed);
      break;
    }
  }

  template<NumericConcept N>
  void set(const N n, Counter& counter, const CommonType type)
  {
    switch (type)
    {
    case CommonType::UInt:
      counter.uint_value.store(static_cast<std::uint64_t>(n), std::memory_order_relaxed);
      break;
    case CommonType::Int:
      counter.int_value.store(static_cast<std::int64_t>(n), std::memory_order_relaxed);
      break;
    case CommonType::Bool:
      counter.bool_value.store(static_cast<bool>(n), std::memory_order_relaxed);
      break;
    case CommonType::Double:
      counter.double_value.store(static_cast<double>(n), std::memory_order_relaxed);
      break;
    }
  }

private:
  const std::string provider_name_;

  const std::optional<std::size_t> initial_size_;

  Statistics statistics_;

  Types types_;

  Names names_;
};

template<
  EnumConcept Enum,
  Internal::Common::EnumConverterConcept<Enum> Converter,
  SharedMutexConcept SharedMutex = std::shared_mutex,
  template<class, class> class Map = std::map>
auto& get_common_statistics_provider(
  const std::string& provider_name = "common",
  std::optional<std::size_t> initial_size = {})
{
  using Provider = CommonStatisticsProvider<Enum, Converter, SharedMutex, Map>;
  static std::shared_ptr<Provider> ptr(new Provider(provider_name, initial_size));
  return ptr;
}

} // namespace UServerUtils::Statistics

#endif //USERVER_STATISTICS_COMMONSTATISTICSPROVIDER_HPP
