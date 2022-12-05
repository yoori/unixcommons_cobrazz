#ifndef SYNC_SEMAPHORE_HPP
#define SYNC_SEMAPHORE_HPP

#include <semaphore.h>

#include <eh/Errno.hpp>

#include <Generics/Uncopyable.hpp>
#include <Generics/Time.hpp>

#include "Condition.hpp"

namespace Sync
{
  class Semaphore : private Generics::Uncopyable
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    explicit
    Semaphore(int count) /*throw (Exception)*/;
    ~Semaphore() throw ();

    void
    acquire() /*throw (Exception)*/;

    bool
    try_acquire() /*throw (Exception)*/;

    bool
    timed_acquire(const Generics::Time* time, bool time_is_relative = false)
      /*throw (Exception)*/;

    void
    release() /*throw (Exception)*/;

    int
    value() /*throw (Exception)*/;

  private:
    Condition condition_lock_;
    int count_;

    //sem_t semaphore_;
  };
}

namespace Sync
{
  /*
  inline
  Semaphore::Semaphore(int count) throw (Exception)
  {
    if (sem_init(&semaphore_, 0, count))
    {
      eh::throw_errno_exception<Exception>(FNE,
        "Failed to initialize semaphore");
    }
  }

  inline
  Semaphore::~Semaphore() throw ()
  {
    sem_destroy(&semaphore_);
  }

  inline
  void
  Semaphore::acquire() throw (Exception)
  {
    for (;;)
    {
      if (!sem_wait(&semaphore_))
      {
        break;
      }
      if (errno != EINTR)
      {
        eh::throw_errno_exception<Exception>(FNE,
          "Failed to wait on semaphore");
      }
    }
  }

  inline
  bool
  Semaphore::try_acquire() throw (Exception)
  {
    if (!sem_trywait(&semaphore_))
    {
      return true;
    }
    if (errno != EAGAIN)
    {
      eh::throw_errno_exception<Exception>(FNE,
        "Failed to wait on semaphore");
    }
    return false;
  }

  inline
  bool
  Semaphore::timed_acquire(const Generics::Time* time,
    bool time_is_relative) throw (Exception)
  {
    if (!time)
    {
      acquire();
      return true;
    }
    Generics::Time real_time(time_is_relative ?
      Generics::Time::get_time_of_day() + *time : *time);
    const timespec RESTRICT =
      { real_time.tv_sec, real_time.tv_usec * 1000 };
    while (sem_timedwait(&semaphore_, &RESTRICT) < 0)
    {
      if (errno == ETIMEDOUT)
      {
        return false;
      }
      if (errno != EINTR)
      {
        eh::throw_errno_exception<Exception>(FNE,
          "Failed to wait on semaphore");
      }
    }
    return true;
  }

  inline
  void
  Semaphore::release() throw (Exception)
  {
    if (sem_post(&semaphore_))
    {
      eh::throw_errno_exception<Exception>(FNE,
        "Failed to release semaphore");
    }
  }

  inline
  int
  Semaphore::value() throw (Exception)
  {
    int value;
    if (sem_getvalue(&semaphore_, &value) < 0)
    {
      eh::throw_errno_exception<Exception>(FNE,
        "Failed to value semaphore");
    }
    return value;
  }
  */

  inline
  Semaphore::Semaphore(int count) /*throw (Exception)*/
    : count_(count)
  {}

  inline
  Semaphore::~Semaphore() throw()
  {}

  inline
  void
  Semaphore::acquire() /*throw (Exception)*/
  {
    ConditionalGuard lock(condition_lock_);

    while(count_ <= 0)
    {
      lock.wait();
    }

    --count_;
  }

  inline
  bool
  Semaphore::try_acquire() /*throw (Exception)*/
  {
    ConditionalGuard lock(condition_lock_);

    if(count_ > 0)
    {
      --count_;
      return true;
    }

    return false;
  }

  inline
  bool
  Semaphore::timed_acquire(
    const Generics::Time* time,
    bool time_is_relative)
    /*throw (Exception)*/
  {
    if(!time)
    {
      acquire();
      return true;
    }

    Generics::Time real_time(time_is_relative ?
      Generics::Time::get_time_of_day() + *time : *time);

    ConditionalGuard lock(condition_lock_);

    while(count_ <= 0)
    {
      if(!lock.timed_wait(&real_time))
      {
        return false;
      }
    }

    --count_;
    return true;
  }

  inline
  void
  Semaphore::release() /*throw (Exception)*/
  {
    ConditionalGuard lock(condition_lock_);
    if(++count_ > 0)
    {
      condition_lock_.signal();
    }
  }

  inline
  int
  Semaphore::value() /*throw (Exception)*/
  {
    ConditionalGuard lock(condition_lock_);
    return count_;
  }
}

#endif
