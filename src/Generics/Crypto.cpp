#include <openssl/crypto.h>

#include <Sync/PosixLock.hpp>

#include <Generics/ArrayAutoPtr.hpp>


namespace
{
  class CryptoLocks : private Generics::Uncopyable
  {
  public:
    CryptoLocks() /*throw (eh::Exception)*/
      : old_locking_function_(CRYPTO_get_locking_callback()),
        old_id_function_(CRYPTO_get_id_callback())
    {
      locks_.reset(CRYPTO_num_locks());

      CRYPTO_set_locking_callback(locking_function_);
      CRYPTO_set_id_callback(id_function_);
    }

    ~CryptoLocks()
    {
      CRYPTO_set_locking_callback(old_locking_function_);
      CRYPTO_set_id_callback(old_id_function_);
    }

  private:
    static
    void
    locking_function_(int mode, int n, const char* /*file*/, int /*line*/) noexcept
    {
      if (mode & CRYPTO_LOCK)
      {
        locks_[n].lock();
      }
      else
      {
        locks_[n].unlock();
      }
    }

    static
    unsigned long
    id_function_() noexcept
    {
      return pthread_self();
    }

    static Generics::ArrayAutoPtr<Sync::PosixMutex> locks_;

    void (*old_locking_function_)(int, int, const char*, int);
    unsigned long (*old_id_function_)();
  };

  Generics::ArrayAutoPtr<Sync::PosixMutex> CryptoLocks::locks_;

  CryptoLocks crypto_locks;
}
