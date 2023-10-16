#ifndef GRPC_CORE_COMMON_QUEUEATOMIC_HPP
#define GRPC_CORE_COMMON_QUEUEATOMIC_HPP

// STD
#include <atomic>
#include <cassert>
#include <memory>

// THIS
#include <Generics/Uncopyable.hpp>

namespace UServerUtils::Grpc::Core::Common
{

namespace Internal
{

#ifndef CACHELINE_SIZE_LOG
#if defined(__i386__) || defined(__x86_64__)
#define CACHELINE_SIZE_LOG 6
#endif
#ifndef CACHELINE_SIZE_LOG
#define CACHELINE_SIZE_LOG 6
#endif /* CACHELINE_SIZE_LOG */
#endif /* CACHELINE_SIZE_LOG */

#define CACHELINE_SIZE (1 << CACHELINE_SIZE_LOG)

class MultiProducerSingleConsumerQueue
{
public:
  struct Node
  {
    std::atomic<Node*> next{nullptr};
  };

  MultiProducerSingleConsumerQueue() noexcept
    : head_{&stub_},
      tail_(&stub_)
  {
  }

  ~MultiProducerSingleConsumerQueue()
  {
    assert(head_.load(std::memory_order_relaxed) == &stub_);
    assert(tail_ == &stub_);
  }

  bool push(Node* node) noexcept
  {
    node->next.store(nullptr, std::memory_order_relaxed);
    Node* prev = head_.exchange(node, std::memory_order_acq_rel);
    prev->next.store(node, std::memory_order_release);
    return prev == &stub_;
  }

  Node* pop() noexcept
  {
    bool empty;
    return pop_and_check_end(&empty);
  }

  // Sets *empty to true if the queue is empty, or false if it is not.
  Node* pop_and_check_end(bool* empty) noexcept
  {
    Node* tail = tail_;
    Node* next = tail_->next.load(std::memory_order_acquire);
    if (tail == &stub_)
    {
      if (next == nullptr)
      {
        *empty = true;
        return nullptr;
      }
      tail_ = next;
      tail = next;
      next = tail->next.load(std::memory_order_acquire);
    }

    if (next != nullptr)
    {
      *empty = false;
      tail_ = next;
      return tail;
    }

    Node* head = head_.load(std::memory_order_acquire);
    if (tail != head)
    {
      *empty = false;
      return nullptr;
    }

    push(&stub_);
    next = tail->next.load(std::memory_order_acquire);
    if (next != nullptr)
    {
      *empty = false;
      tail_ = next;
      return tail;
    }
    *empty = false;
    return nullptr;
  }

private:
  union
  {
    char padding_[CACHELINE_SIZE];
    std::atomic<Node*> head_{nullptr};
  };

  Node* tail_;
  Node stub_;
};

} // namespace Internal

template<class T>
class QueueAtomic final : protected Generics::Uncopyable
{
private:
  struct Node :
    public Internal::MultiProducerSingleConsumerQueue::Node,
    protected Generics::Uncopyable
  {
    template<
      class First,
      class ...Args,
      typename = std::enable_if_t<!std::is_same_v<std::decay_t<First>, Node>>>
    Node(First&& first, Args&& ...args)
      : data(std::make_unique<T>(
          std::forward<First>(first),
          std::forward<Args>(args)...))
    {
    }

    std::unique_ptr<T> data;
  };

public:
  explicit QueueAtomic(
    const std::int32_t max_size = 1000000) noexcept
    : max_size_(max_size)
  {
  }

  ~QueueAtomic()
  {
    try
    {
      clear();
    }
    catch (...)
    {
    }
  }

  /**
   * Add an element to the queue.
   * If return false - element was not added.
   * If false or exception is thrown, this function has no effect
   **/
  template<class... Args>
  bool emplace(Args&&... args)
  {
    static_assert(
      std::is_constructible_v<T, Args&&...>,
      "T must be constructible by types Args");

    if (counter_.load(std::memory_order_relaxed) > max_size_)
    {
      return false;
    }

    std::unique_ptr<Node> data = std::make_unique<Node>(
      std::forward<Args>(args)...);
    queue_.push(static_cast<Node*>(data.release()));
    counter_.fetch_add(1, std::memory_order_relaxed);

    return true;
  }

  std::unique_ptr<T> pop() noexcept
  {
    bool empty;
    std::unique_ptr<Node> node(
      static_cast<Node*>(queue_.pop_and_check_end(&empty)));
    if (empty || node)
    {
      if (node)
      {
        counter_.fetch_sub(1, std::memory_order_relaxed);
        return std::move(node->data);
      }
      else
      {
        return {};
      }
    }
    else
    {
      do
      {
        node.reset(static_cast<Node*>(
          queue_.pop_and_check_end(&empty)));
      }
      while (!node && !empty);

      if (node)
      {
        counter_.fetch_sub(1, std::memory_order_relaxed);
        return std::move(node->data);
      }
      else
      {
        return {};
      }
    }
  }

  void clear()
  {
    bool empty = false;
    do
    {
      auto* ptr = queue_.pop_and_check_end(&empty);
      std::unique_ptr<Node> data(static_cast<Node*>(ptr));
    }
    while (!empty);
    counter_.exchange(0, std::memory_order_relaxed);
  }

private:
  const std::int32_t max_size_;

  std::atomic<std::int32_t> counter_{0};

  Internal::MultiProducerSingleConsumerQueue queue_;
};

} // UServerUtils::Grpc::Core::Common

#endif //GRPC_CORE_COMMON_QUEUEATOMIC_HPP