#ifndef USERVER_STATISTICS_CONCEPT_HPP
#define USERVER_STATISTICS_CONCEPT_HPP

namespace UServerUtils::Statistics
{

template<class T>
concept EnumConcept = std::is_enum_v<T> && requires(T t)
{
  T::Max;
};

template<class T>
concept SharedMutexConcept =
  std::is_object_v<T> &&
  std::is_default_constructible_v<T> &&
  requires(T t)
{
  t.lock();
  t.unlock();
  t.try_lock();
  t.lock_shared();
  t.unlock_shared();
  t.try_lock_shared();
};

template<class T, class... U>
concept IsAnyOfConcept = (std::same_as<T, U> || ...);

template<typename T>
concept NumericConcept = std::is_arithmetic_v<T>;

template<class T, class... U>
concept IsConvertibleAnyConcept = (std::is_convertible_v<T, U> || ...);

} // namespace UServerUtils::Statistics

#endif //USERVER_STATISTICS_CONCEPT_HPP
