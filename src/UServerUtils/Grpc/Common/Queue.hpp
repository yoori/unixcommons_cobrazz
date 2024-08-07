#ifndef GRPC_COMMON_QUEUE_H_
#define GRPC_COMMON_QUEUE_H_

// STD
#include <deque>
#include <memory>
#include <mutex>

// THIS
#include <Generics/Uncopyable.hpp>

namespace UServerUtils::Grpc::Common
{

template<class T>
class Queue final : protected Generics::Uncopyable
{
public:
  explicit Queue(const std::size_t max_size = 10000)
    : max_size_(max_size)
  {
  }

  ~Queue() = default;

  template<class... Args>
  bool emplace(Args&&... args)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.size() > max_size_)
    {
      return false;
    }

    queue_.emplace_back(
      std::make_unique<T>(
        std::forward<Args>(args)...));
    return true;
  }

  std::unique_ptr<T> pop()
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty())
      return {};

    auto data = std::move(queue_.front());
    queue_.pop_front();
    return data;
  }

  void clear()
  {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.clear();
  }

private:
  const std::size_t max_size_;

  std::deque<std::unique_ptr<T>> queue_;

  mutable std::mutex mutex_;
};

} // namespace UServerUtils::Grpc::Common

#endif // GRPC_COMMON_QUEUE_H_