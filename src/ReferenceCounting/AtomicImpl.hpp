/**
 * @file AtomicImpl.hpp
 * @author Konstantin Sadov [Konstantin_Sadov@ocslab.com]
 * Interface of AtomicImpl class. Reference counting base
 * class using atomic arithmetic functions
 */
#ifndef REFERENCE_COUNTING_ATOMIC_IMPL_HPP
#define REFERENCE_COUNTING_ATOMIC_IMPL_HPP

#include <cassert>

#include <Generics/AtomicInt.hpp>
#include <ReferenceCounting/Interface.hpp>

namespace ReferenceCounting
{
  /**
   * For performance purposes usage of atomic arithmetic
   * functions are required for reference counting
   */
  class AtomicImpl :
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
    AtomicImpl() noexcept;
    virtual
    ~AtomicImpl() noexcept;

    virtual
    bool
    remove_ref_no_delete_() const noexcept;

    virtual
    void
    delete_this_() const noexcept;

  protected:
    mutable Generics::AtomicInt ref_count_;
  };

  class AtomicCopyImpl : public AtomicImpl
  {
  protected:
    AtomicCopyImpl() noexcept;
    AtomicCopyImpl(const volatile AtomicCopyImpl&) noexcept;
    virtual
    ~AtomicCopyImpl() noexcept;
  };
}

namespace ReferenceCounting
{
  inline
  AtomicImpl::AtomicImpl() noexcept
    : ReferenceCounting::Interface(), ref_count_(1)
  {
  }

  inline
  AtomicImpl::~AtomicImpl() noexcept
  {
#ifndef NVALGRIND
    RunningOnValgrind<>::check_ref_count(ref_count_);
#endif
  }

  inline
  void
  AtomicImpl::add_ref() const noexcept
  {
    ++ref_count_;
  }

  inline
  void
  AtomicImpl::delete_this_() const noexcept
  {
    delete this;
  }

  inline
  bool
  AtomicImpl::remove_ref_no_delete_() const noexcept
  {
    int old = ref_count_.exchange_and_add(-1);
    assert(old > 0);
    return old == 1;
  }

  inline
  void
  AtomicImpl::remove_ref() const noexcept
  {
    if (remove_ref_no_delete_())
    {
      delete_this_();
    }
  }


  inline
  AtomicCopyImpl::AtomicCopyImpl() noexcept
    : ReferenceCounting::Interface()
  {
  }

  inline
  AtomicCopyImpl::AtomicCopyImpl(const volatile AtomicCopyImpl&) noexcept
    : ReferenceCounting::Interface(), AtomicImpl()
  {
  }

  inline
  AtomicCopyImpl::~AtomicCopyImpl() noexcept
  {
  }
}

#endif
