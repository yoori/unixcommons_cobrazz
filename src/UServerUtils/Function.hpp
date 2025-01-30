#ifndef USERVER_FUNCTION_HPP
#define USERVER_FUNCTION_HPP

// STD
#include <functional>
#include <type_traits>

namespace UServerUtils::Utils
{

template<class T>
class Function;

template<class R, class ...Args>
class Function<R(Args...)> final
{
public:
  using Func = std::function<R(Args...)>;

public:
  template<class F,
    class = std::enable_if_t<
      !std::is_same_v<Function, std::decay_t<F>> &&
       std::is_invocable_v<std::decay_t<F>, Args...>>>
  Function(F&& f)
  {
    create(std::forward<F>(f));
  }

  template<class F,
    class = std::enable_if_t<
      !std::is_same_v<Function, std::decay_t<F>> &&
      std::is_invocable_v<std::decay_t<F>, Args...>>>
  Function& operator=(F&& f)
  {
    create(std::forward<F>(f));
    return *this;
  }

  Function() = default;

  ~Function() = default;

  Function(const Function&) = default;
  Function(Function&&) = default;
  Function& operator=(const Function&) = default;
  Function& operator=(Function&&) = default;

  R operator()(Args... args)
  {
    return func_(std::forward<Args>(args)...);
  }

  operator bool() const noexcept
  {
    return func_.operator bool();
  }

private:
  template<class F>
  void create(F&& f)
  {
    auto ptr = std::make_shared<std::decay_t<F>>(
      std::forward<F>(f));
    func_ = [ptr = std::move(ptr)] (Args... args) {
      return (*ptr)(std::forward<Args>(args)...);
    };
  }

private:
  Func func_;
};

} // namespace UServerUtils::Utils

#endif // USERVER_FUNCTION_HPP