// file      : ReferenceCounting/DefaultImpl.hpp
// author    : Boris Kolpackov <boris@kolpackov.net>
// copyright : Copyright (c) 2002-2003 Boris Kolpackov
// license   : http://kolpackov.net/license.html

#ifndef REFERENCE_COUNTING_DEFAULT_IMPL_HPP
#define REFERENCE_COUNTING_DEFAULT_IMPL_HPP

#include <cassert>

#include <Sync/NullSynchPolicy.hpp>

#include <ReferenceCounting/Interface.hpp>


namespace ReferenceCounting
{
  // Default reference counter implementation with parameterized
  // synchronization policy. It is assumed that none of the SynchPolicy
  // types throw any logic exceptions. If in fact they do then these
  // exceptions won't be handled and will be automatically converted
  // to system exceptions.

  template <typename SynchPolicy = Sync::Policy::Null>
  class DefaultImpl :
    public virtual Interface,
    private Generics::Uncopyable
  {
  public:
    virtual
    void
    add_ref() const noexcept;

    virtual
    void
    remove_ref() const noexcept;

  protected:
    DefaultImpl() noexcept;
    virtual
    ~DefaultImpl() noexcept;


  private:
    typedef typename SynchPolicy::Mutex Mutex;
    typedef typename SynchPolicy::WriteGuard Guard;

    mutable Mutex lock_;
    mutable unsigned long ref_count_;
  };
}

namespace ReferenceCounting
{
  template <typename SynchPolicy>
  DefaultImpl<SynchPolicy>::DefaultImpl() noexcept
    : ReferenceCounting::Interface(), lock_(), ref_count_(1)
  {
  }

  template <typename SynchPolicy>
  DefaultImpl<SynchPolicy>::~DefaultImpl() noexcept
  {
#ifndef NVALGRIND
    RunningOnValgrind<>::check_ref_count(ref_count_);
#endif
  }

  template <typename SynchPolicy>
  void
  DefaultImpl<SynchPolicy>::add_ref() const noexcept
  {
    Guard guard(lock_);
    ref_count_++;
  }

  template <typename SynchPolicy>
  void
  DefaultImpl<SynchPolicy>::remove_ref() const noexcept
  {
    bool delete_this;

    {
      Guard guard(lock_);

      assert(ref_count_ > 0);
      delete_this = !--ref_count_;
    }

    if (delete_this)
    {
      delete this;
    }
  }
}

#endif
