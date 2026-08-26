#pragma once

#include <pthread.h>

#include <Generics/Uncopyable.hpp>


namespace Sync
{
  class PosixMutex : private Generics::Uncopyable
  {
  public:
    constexpr
    PosixMutex() noexcept;

    /**
     * Create mutex and set pshared attribute
     * @param pshared value to do system call pthread_mutexattr_setpshared
     */
    explicit
    PosixMutex(int pshared) noexcept;

    ~PosixMutex() noexcept;

    operator pthread_mutex_t&() noexcept;

    void
    lock() noexcept;

    void
    unlock() noexcept;

  private:
    pthread_mutex_t mutex_;
  };

  class PosixGuard : private Generics::Uncopyable
  {
  public:
    explicit
    PosixGuard(pthread_mutex_t& mutex) noexcept;
    ~PosixGuard() noexcept;

  private:
    pthread_mutex_t& mutex_;
  };

  class PosixTryGuard : private Generics::Uncopyable
  {
  public:
    explicit
    PosixTryGuard(pthread_mutex_t& mutex) noexcept;
    ~PosixTryGuard() noexcept;
    operator bool() const noexcept;

  private:
    pthread_mutex_t& mutex_;
    bool locked_;
  };

  class PosixRWLock : private Generics::Uncopyable
  {
  public:
    constexpr
    PosixRWLock() noexcept;
    ~PosixRWLock() noexcept;
    operator pthread_rwlock_t&() noexcept;
    void
    lock_read() noexcept;
    void
    lock_write() noexcept;
    void
    unlock() noexcept;

  private:
    pthread_rwlock_t lock_;
  };

  class PosixRGuard : private Generics::Uncopyable
  {
  public:
    explicit
    PosixRGuard(pthread_rwlock_t& lock) noexcept;
    ~PosixRGuard() noexcept;

  private:
    pthread_rwlock_t& lock_;
  };

  class PosixWGuard : private Generics::Uncopyable
  {
  public:
    explicit
    PosixWGuard(pthread_rwlock_t& lock) noexcept;
    ~PosixWGuard() noexcept;

  private:
    pthread_rwlock_t& lock_;
  };

  class PosixSpinLock : private Generics::Uncopyable
  {
  public:
    /**
     * Create spinlock and set pshared attribute
     * @param pshared value to do system call pthread_spin_init
     */
    explicit
    PosixSpinLock(int pshared = PTHREAD_PROCESS_PRIVATE) noexcept;

    ~PosixSpinLock() noexcept;

    operator pthread_spinlock_t&() noexcept;

    void
    lock() noexcept;

    void
    unlock() noexcept;

  private:
    pthread_spinlock_t spinlock_;
  };

  class PosixSpinGuard : private Generics::Uncopyable
  {
  public:
    explicit
    PosixSpinGuard(pthread_spinlock_t& mutex) noexcept;
    ~PosixSpinGuard() noexcept;

  private:
    pthread_spinlock_t& spinlock_;
  };
}

//
// INLINES
//

namespace Sync
{
  //
  // PosixMutex class
  //

  inline
  constexpr
  PosixMutex::PosixMutex() noexcept
    : mutex_ PTHREAD_MUTEX_INITIALIZER
  {
  }

  inline
  PosixMutex::PosixMutex(int pshared) noexcept
  {
    pthread_mutexattr_t mutex_attributes;
    pthread_mutexattr_init(&mutex_attributes);
    pthread_mutexattr_setpshared(&mutex_attributes, pshared);
    pthread_mutex_init(&mutex_, &mutex_attributes);
  }

  inline
  PosixMutex::~PosixMutex() noexcept
  {
    pthread_mutex_destroy(&mutex_);
  }

  inline
  PosixMutex::operator pthread_mutex_t&() noexcept
  {
    return mutex_;
  }

  inline
  void
  PosixMutex::lock() noexcept
  {
    pthread_mutex_lock(&mutex_);
  }

  inline
  void
  PosixMutex::unlock() noexcept
  {
    pthread_mutex_unlock(&mutex_);
  }


  //
  // PosixGuard class
  //

  inline
  PosixGuard::PosixGuard(pthread_mutex_t& mutex) noexcept
    : mutex_(mutex)
  {
    pthread_mutex_lock(&mutex_);
  }

  inline
  PosixGuard::~PosixGuard() noexcept
  {
    pthread_mutex_unlock(&mutex_);
  }


  //
  // PosixTryGuard class
  //

  inline
  PosixTryGuard::PosixTryGuard(pthread_mutex_t& mutex) noexcept
    : mutex_(mutex), locked_(!pthread_mutex_trylock(&mutex_))
  {
  }

  inline
  PosixTryGuard::~PosixTryGuard() noexcept
  {
    if (locked_)
    {
      pthread_mutex_unlock(&mutex_);
    }
  }

  inline
  PosixTryGuard::operator bool() const noexcept
  {
    return locked_;
  }


  //
  // PosixRWLock class
  //

  inline
  constexpr
  PosixRWLock::PosixRWLock() noexcept
    : lock_ PTHREAD_RWLOCK_INITIALIZER
  {
  }

  inline
  PosixRWLock::~PosixRWLock() noexcept
  {
    pthread_rwlock_destroy(&lock_);
  }

  inline
  PosixRWLock::operator pthread_rwlock_t&() noexcept
  {
    return lock_;
  }

  inline
  void
  PosixRWLock::lock_read() noexcept
  {
    pthread_rwlock_rdlock(&lock_);
  }

  inline
  void
  PosixRWLock::lock_write() noexcept
  {
    pthread_rwlock_wrlock(&lock_);
  }

  inline
  void
  PosixRWLock::unlock() noexcept
  {
    pthread_rwlock_unlock(&lock_);
  }


  //
  // PosixRGuard class
  //

  inline
  PosixRGuard::PosixRGuard(pthread_rwlock_t& lock) noexcept
    : lock_(lock)
  {
    pthread_rwlock_rdlock(&lock_);
  }

  inline
  PosixRGuard::~PosixRGuard() noexcept
  {
    pthread_rwlock_unlock(&lock_);
  }


  //
  // PosixWGuard class
  //

  inline
  PosixWGuard::PosixWGuard(pthread_rwlock_t& lock) noexcept
    : lock_(lock)
  {
    pthread_rwlock_wrlock(&lock_);
  }

  inline
  PosixWGuard::~PosixWGuard() noexcept
  {
    pthread_rwlock_unlock(&lock_);
  }


  //
  // PosixSpinLock class
  //

  inline
  PosixSpinLock::PosixSpinLock(int pshared) noexcept
  {
    pthread_spin_init(&spinlock_, pshared);
  }

  inline
  PosixSpinLock::~PosixSpinLock() noexcept
  {
    pthread_spin_destroy(&spinlock_);
  }

  inline
  PosixSpinLock::operator pthread_spinlock_t& () noexcept
  {
    return spinlock_;
  }

  inline
  void
  PosixSpinLock::lock() noexcept
  {
    pthread_spin_lock(&spinlock_);
  }

  inline
  void
  PosixSpinLock::unlock() noexcept
  {
    pthread_spin_unlock(&spinlock_);
  }


  //
  // PosixSpinGuard class
  //

  inline
  PosixSpinGuard::PosixSpinGuard(pthread_spinlock_t& spinlock) noexcept
    : spinlock_(spinlock)
  {
    pthread_spin_lock(&spinlock_);
  }

  inline
  PosixSpinGuard::~PosixSpinGuard() noexcept
  {
    pthread_spin_unlock(&spinlock_);
  }
}
