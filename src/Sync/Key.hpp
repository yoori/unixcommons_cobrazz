#ifndef SYNC_KEY_HPP
#define SYNC_KEY_HPP

#include <pthread.h>

#include <eh/Errno.hpp>

#include <Generics/Uncopyable.hpp>
#include <Generics/Function.hpp>


namespace Sync
{
  /**
   * Performs access to thread-specific data stored as pointers
   */
  template <typename Data>
  class Key : private Generics::Uncopyable
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    /**
     * Create thread-specific key
     * @param destructor optional destructor called for data on thread
     * termination
     */
    explicit
    Key(void (*destructor)(void*) = 0) /*throw (Exception)*/;

    /**
     * Store data for the current thread
     * @param data to store
     */
    void
    set_data(Data* data) /*throw (Exception)*/;

    /**
     * Get stored data for the current thread
     * @return stored data
     */
    Data*
    get_data() throw ();

  private:
    pthread_key_t key_;
  };
}

namespace Sync
{
  template <typename Data>
  Key<Data>::Key(void (*destructor)(void*)) /*throw (Exception)*/
  {
    const int RES = pthread_key_create(&key_, destructor);
    if (RES)
    {
      eh::throw_errno_exception<Exception>(RES, FNE,
        "Failed to create key");
    }
  }

  template <typename Data>
  void
  Key<Data>::set_data(Data* data) /*throw (Exception)*/
  {
    const int RES = pthread_setspecific(key_, data);
    if (RES)
    {
      eh::throw_errno_exception<Exception>(RES, FNE,
        "Failed to set data");
    }
  }

  template <typename Data>
  Data*
  Key<Data>::get_data() throw ()
  {
    return static_cast<Data*>(pthread_getspecific(key_));
  }
}

#endif
