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
    add_ref() const throw ();

    virtual
    void
    remove_ref() const throw ();

  protected:
    AtomicImpl() throw ();
    virtual
    ~AtomicImpl() throw ();

    virtual
    bool
    remove_ref_no_delete_() const throw ();

    virtual
    void
    delete_this_() const throw ();

  protected:
    mutable Generics::AtomicInt ref_count_;
  };

  class AtomicCopyImpl : public AtomicImpl
  {
  protected:
    AtomicCopyImpl() throw ();
    AtomicCopyImpl(const volatile AtomicCopyImpl&) throw ();
    virtual
    ~AtomicCopyImpl() throw ();
  };
}

namespace ReferenceCounting
{
  inline
  AtomicImpl::AtomicImpl() throw ()
    : ReferenceCounting::Interface(), ref_count_(1)
  {
  }

  inline
  AtomicImpl::~AtomicImpl() throw ()
  {
#ifndef NVALGRIND
    RunningOnValgrind<>::check_ref_count(ref_count_);
#endif
  }

  inline
  void
  AtomicImpl::add_ref() const throw ()
  {
    ++ref_count_;
  }

  inline
  void
  AtomicImpl::delete_this_() const throw ()
  {
    delete this;
  }

  inline
  bool
  AtomicImpl::remove_ref_no_delete_() const throw ()
  {
    int old = ref_count_.exchange_and_add(-1);
    assert(old > 0);
    return old == 1;
  }

  inline
  void
  AtomicImpl::remove_ref() const throw ()
  {
    if (remove_ref_no_delete_())
    {
      delete_this_();
    }
  }


  inline
  AtomicCopyImpl::AtomicCopyImpl() throw ()
    : ReferenceCounting::Interface()
  {
  }

  inline
  AtomicCopyImpl::AtomicCopyImpl(const volatile AtomicCopyImpl&) throw ()
    : ReferenceCounting::Interface(), AtomicImpl()
  {
  }

  inline
  AtomicCopyImpl::~AtomicCopyImpl() throw ()
  {
  }
}

#endif
