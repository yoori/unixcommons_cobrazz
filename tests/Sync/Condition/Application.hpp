// Application.hpp
#pragma once

#include <vector>
#include <Sync/Condition.hpp>
#include <Sync/PosixLock.hpp>

class ConsumerProducer
{
public:
  ConsumerProducer(std::size_t max_item_count,
    std::size_t producer_threads_count = 1)
    /*throw(Sync::Conditional::Exception)*/;
  ~ConsumerProducer() noexcept;

  void
  producer(std::size_t &work_stat)
    /*throw(Sync::Conditional::Exception)*/;

  static void *
  producer(void *arg) noexcept;

  void
  consumer() /*throw(Sync::Conditional::Exception)*/;

  static void *
  consumer(void *arg) noexcept;

private:
  const std::size_t MAX_ITEM_COUNT_;

  std::vector<std::size_t> buffer_;   // Producers fill this buffer
  std::size_t next_value_;
  std::size_t ready_number_;

  Sync::PosixMutex mutex_;  // Data ^ protection

  Sync::Condition cond_;

  struct ThreadContext
  {
    ThreadContext(ConsumerProducer *this_ptr_val) noexcept;
    ConsumerProducer *this_ptr;
    std::size_t work_done_stat;
    pthread_t thread;
  };

  typedef std::vector<ThreadContext> ThreadsContainer;
  ThreadsContainer threads_;
};
