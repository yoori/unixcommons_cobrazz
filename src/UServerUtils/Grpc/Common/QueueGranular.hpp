#ifndef GRPC_COMMON_QUEUE_GRANULAR_H_
#define GRPC_COMMON_QUEUE_GRANULAR_H_

// STD
#include <atomic>
#include <memory>
#include <mutex>

// THIS
#include <Generics/Uncopyable.hpp>

namespace UServerUtils::Grpc::Common
{

template<class T>
class QueueGranular final : protected Generics::Uncopyable
{
private:
  struct Node
  {
    std::unique_ptr<T> data;
    std::unique_ptr<Node> next;
  };

public:
  explicit QueueGranular(const std::int32_t max_size = 10000)
    : max_size_(max_size),
      head_(std::make_unique<Node>()),
      tail_(head_.get())
  {
  }

  ~QueueGranular() = default;

  template<class... Args>
  bool emplace(Args&&... args)
  {
    if (counter_.load(std::memory_order_relaxed) > max_size_)
    {
      return false;
    }

    {
      std::unique_ptr <T> data = std::make_unique<T>(
        std::forward<Args>(args)...);
      std::unique_ptr <Node> node = std::make_unique<Node>();

      std::lock_guard <std::mutex> tail_lock(tail_mutex_);
      tail_->data = std::move(data);
      Node *const new_tail = node.get();
      tail_->next = std::move(node);
      tail_ = new_tail;
    }
    counter_.fetch_add(1, std::memory_order_relaxed);

    return true;
  }

  std::unique_ptr<T> pop()
  {
    std::unique_ptr<Node> old_head = try_pop_head();
    if (!old_head)
      return {};

    counter_.fetch_sub(1, std::memory_order_relaxed);
    auto data = std::move(old_head->data);
    return data;
  }

  void clear()
  {

    std::lock_guard<std::mutex> head_lock(head_mutex_);
    std::lock_guard<std::mutex> tail_lock(tail_mutex_);

    head_ = std::make_unique<Node>();
    tail_ = head_.get();
    counter_.exchange(0, std::memory_order_relaxed);
  }

private:
  Node* get_tail()
  {
    std::lock_guard<std::mutex> tail_lock(tail_mutex_);
    return tail_;
  }

  std::unique_ptr<Node> pop_head()
  {
    std::unique_ptr<Node> old_head = std::move(head_);
    head_ = std::move(old_head->next);
    return old_head;
  }

  std::unique_ptr<Node> try_pop_head()
  {
    std::lock_guard<std::mutex> head_lock(head_mutex_);
    if (head_.get() == get_tail())
    {
      return std::unique_ptr<Node>();
    }
    return pop_head();
  }

private:
  const std::int32_t max_size_;

  std::atomic<std::int32_t> counter_{0};

  std::mutex head_mutex_;

  std::unique_ptr<Node> head_;

  std::mutex tail_mutex_;

  Node* tail_;
};

} // namespace UServerUtils::Grpc::Common

#endif // GRPC_COMMON_QUEUE_GRANULAR_H_
