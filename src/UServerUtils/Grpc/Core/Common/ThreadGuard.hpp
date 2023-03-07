#ifndef GRPC_CORE_COMMON_THREAD_GUARD_H_
#define GRPC_CORE_COMMON_THREAD_GUARD_H_

// STD
#include <mutex>
#include <deque>
#include <thread>

namespace UServerUtils::Grpc::Core::Common
{

class ThreadGuard final
{
public:
  explicit ThreadGuard(std::thread&& thread) noexcept
    : thread_(std::move(thread))
  {
  }

  template<class Function, class... Args>
  explicit ThreadGuard(Function&& function, Args&&... args)
    : thread_(
      std::forward<Function>(function),
      std::forward<Args>(args)...)
  {
  }

  ~ThreadGuard()
  {
    try
    {
      if (thread_.joinable())
      {
        thread_.join();
      }
    }
    catch (...)
    {
    }
  }

  ThreadGuard(const ThreadGuard&) = delete;
  ThreadGuard& operator=(const ThreadGuard&) = delete;
  ThreadGuard(ThreadGuard&& threadGuard) = default;
  ThreadGuard& operator=(ThreadGuard&&) = default;

private:
  std::thread thread_;
};

class ThreadsGuard final
{
public:
  explicit ThreadsGuard() = default;

  ~ThreadsGuard()
  {
    try
    {
      clear();
    }
    catch (...)
    {
    }
  }

  ThreadsGuard(const ThreadsGuard&) = delete;
  ThreadsGuard& operator=(const ThreadsGuard&) = delete;
   ThreadsGuard(ThreadsGuard&&) = default;
  ThreadsGuard& operator=(ThreadsGuard&&) = default;

  template<class Function, class... Args>
  void add(Function&& function, Args&&... args)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    threads_.emplace_back(
      std::forward<Function>(function),
      std::forward<Args>(args)...);
  }

  void clear()
  {
    std::lock_guard<std::mutex> lock(mutex_);
    threads_.clear();
  }

  std::size_t size() const
  {
    std::lock_guard<std::mutex> lock(mutex_);
    return threads_.size();
  }

  bool empty() const
  {
    std::lock_guard<std::mutex> lock(mutex_);
    return threads_.empty();
  }

private:
  mutable std::mutex mutex_;

  std::deque<ThreadGuard> threads_;
};

} // namespace UServerUtils::Grpc::Core::Common

#endif // GRPC_CORE_COMMON_THREAD_GUARD_H_
