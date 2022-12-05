// Application.hpp
#ifndef __TEST_APPLICATION_CONDITION_HPP_INCLUDED__
#define __TEST_APPLICATION_CONDITION_HPP_INCLUDED__ 

#include <vector>
#include <Sync/Condition.hpp>
#include <Sync/PosixLock.hpp>

class ConsumerProducer
{
public:
  ConsumerProducer(std::size_t max_item_count,
    std::size_t producer_threads_count = 1)
    /*throw(Sync::Conditional::Exception)*/;
  ~ConsumerProducer() throw();

  void
  producer(std::size_t &work_stat)
    /*throw(Sync::Conditional::Exception)*/;

  static void *
  producer(void *arg) throw();

  void
  consumer() /*throw(Sync::Conditional::Exception)*/;

  static void *
  consumer(void *arg) throw();

private:
  const std::size_t MAX_ITEM_COUNT_;

  std::vector<std::size_t> buffer_;   // Producers fill this buffer
  std::size_t next_value_;
  std::size_t ready_number_;

  Sync::PosixMutex mutex_;  // Data ^ protection

  Sync::Condition cond_;

  struct ThreadContext
  {
    ThreadContext(ConsumerProducer *this_ptr_val) throw();
    ConsumerProducer *this_ptr;
    std::size_t work_done_stat;
    pthread_t thread;
  };

  typedef std::vector<ThreadContext> ThreadsContainer;
  ThreadsContainer threads_;
};

#endif  // __TEST_APPLICATION_CONDITION_HPP_INCLUDED__
