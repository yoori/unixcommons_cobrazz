#ifndef REFERENCE_COUNTING_HPP
#define REFERENCE_COUNTING_HPP

#include <Sync/PosixLock.hpp>

#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>


namespace ReferenceCounting
{
  /*
   * Weak functionality to break cyclic dependencies.
   * Usage:
   * class TheClass: public WeakImpl or its derived class
   * {
   *   ...
   * };
   * typedef QualPtr<TheClass> TheClass_var;
   * typedef QualPtr<WeakPtr<TheClass>> TheClassWeakPtr;
   * ...
   * TheClass_var obj(new TheClass(...));
   * ...
   * TheClassWeakPtr weak(ReferenceCounting::weak_ptr(obj));
   * ...
   * TheClass_var ptr(weak.get());
   * if (ptr)
   * {
   *   ptr->...();
   * }
   */

  struct RCSpin :
    private Sync::PosixSpinLock,
    public AtomicImpl
  {
  public:
    using Sync::PosixSpinLock::operator pthread_spinlock_t&;

  protected:
    virtual
    ~RCSpin() throw () = default;
  };
  typedef SmartPtr<RCSpin, PolicyAssert> RCSpin_var;

  struct WeakPtrBase;

  class WeakImpl :
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
    WeakImpl() throw ();
    virtual
    ~WeakImpl() throw ();

    virtual
    bool
    remove_ref_no_delete_() const throw ();

    virtual
    void
    delete_this_() const throw ();

  private:
    RCSpin*
    add_weak_ptr_(WeakPtrBase* weak_ptr) const throw ();

    void
    remove_weak_ptr_(const WeakPtrBase* weak_ptr) const throw ();

    mutable RCSpin_var lock_;
    mutable unsigned ref_count_;
    mutable WeakPtrBase* weak_ptr_;

    friend struct WeakPtrBase;
  };

  struct WeakPtrBase
  {
    WeakPtrBase* next;

    virtual
    void
    clear_ptr() throw () = 0;

    RCSpin*
    add_ptr(const WeakImpl* obj) throw ();

    void
    remove_ptr(const WeakImpl* obj) const throw ();

    void
    ptr_add_ref(WeakImpl* obj) const throw ();
  };

  template <typename T>
  class WeakPtr :
    public AtomicImpl,
    private WeakPtrBase
  {
  public:
    explicit
    WeakPtr(T* object) throw ();

    FixedPtr<T>
    get() throw ();

    FixedPtr<const T>
    get() const throw ();

  private:
    virtual
    ~WeakPtr() throw () = default;

    virtual
    void
    delete_this_() const throw ();

    virtual
    void
    clear_ptr() throw ();

    RCSpin_var lock_;
    T* object_;
  };

  template <template <typename, typename> class SmartPtr = QualPtr,
    typename Policy = PolicyAssert, typename T = WeakImpl>
  SmartPtr<WeakPtr<T>, Policy>
  weak_ptr(T* object) /*throw (eh::Exception)*/;

  template <template <typename, typename> class SmartPtr = QualPtr,
    typename Policy = PolicyAssert, typename T = void>
  SmartPtr<WeakPtr<typename std::remove_reference<T>::type::Type>,
    Policy>
  weak_ptr(T&& object) /*throw (eh::Exception)*/;
}

namespace ReferenceCounting
{
  //
  // WeakImpl class
  //

  inline
  WeakImpl::WeakImpl() throw ()
    : ReferenceCounting::Interface(), lock_(new RCSpin),
      ref_count_(1), weak_ptr_(nullptr)
  {
  }

  inline
  WeakImpl::~WeakImpl() throw ()
  {
#ifndef NVALGRIND
    RunningOnValgrind<>::check_ref_count(ref_count_);
#endif
  }

  inline
  void
  WeakImpl::add_ref() const throw ()
  {
    Sync::PosixSpinGuard guard(*lock_);
    ref_count_++;
  }

  inline
  bool
  WeakImpl::remove_ref_no_delete_() const throw ()
  {
    Sync::PosixSpinGuard guard(*lock_);
    if (!--ref_count_)
    {
      for (; weak_ptr_; weak_ptr_ = weak_ptr_->next)
      {
        weak_ptr_->clear_ptr();
      }
      return true;
    }
    return false;
  }

  inline
  void
  WeakImpl::remove_ref() const throw ()
  {
    if (remove_ref_no_delete_())
    {
      delete_this_();
    }
  }

  inline
  void
  WeakImpl::delete_this_() const throw ()
  {
    delete this;
  }

  inline
  RCSpin*
  WeakImpl::add_weak_ptr_(WeakPtrBase* weak_ptr) const throw ()
  {
    Sync::PosixSpinGuard guard(*lock_);
    weak_ptr->next = weak_ptr_;
    weak_ptr_ = weak_ptr;
    return lock_;
  }

  inline
  void
  WeakImpl::remove_weak_ptr_(const WeakPtrBase* weak_ptr) const throw ()
  {
    for (WeakPtrBase** wp = &weak_ptr_; *wp; wp = &(*wp)->next)
    {
      if (*wp == weak_ptr)
      {
        *wp = (*wp)->next;
        return;
      }
    }
  }


  //
  // WeakPtrBase class
  //

  inline
  RCSpin*
  WeakPtrBase::add_ptr(const WeakImpl* obj) throw ()
  {
    return obj->add_weak_ptr_(this);
  }

  inline
  void
  WeakPtrBase::remove_ptr(const WeakImpl* obj) const throw ()
  {
    if (obj)
    {
      obj->remove_weak_ptr_(this);
    }
  }

  inline
  void
  WeakPtrBase::ptr_add_ref(WeakImpl* obj) const throw ()
  {
    if (obj)
    {
      obj->ref_count_++;
    }
  }


  //
  // WeakPtr class
  //

  template <typename T>
  WeakPtr<T>::WeakPtr(T* object) throw ()
    : lock_(ReferenceCounting::add_ref(WeakPtrBase::add_ptr(object))),
      object_(object)
  {
  }

  template <typename T>
  void
  WeakPtr<T>::delete_this_() const throw ()
  {
    {
      Sync::PosixSpinGuard guard(*lock_);
      WeakPtrBase::remove_ptr(object_);
    }
    AtomicImpl::delete_this_();
  }

  template <typename T>
  FixedPtr<T>
  WeakPtr<T>::get() throw ()
  {
    Sync::PosixSpinGuard guard(*lock_);
    WeakPtrBase::ptr_add_ref(object_);
    return object_;
  }

  template <typename T>
  FixedPtr<const T>
  WeakPtr<T>::get() const throw ()
  {
    Sync::PosixSpinGuard guard(*lock_);
    WeakPtrBase::ptr_add_ref(object_);
    return object_;
  }

  template <typename T>
  void
  WeakPtr<T>::clear_ptr() throw ()
  {
    object_ = nullptr;
  }


  template <template <typename, typename> class SmartPtr,
    typename Policy, typename T>
  SmartPtr<WeakPtr<T>, Policy>
  weak_ptr(T* object) /*throw (eh::Exception)*/
  {
    assert(object);
    return new WeakPtr<T>(object);
  }

  template <template <typename, typename> class SmartPtr,
    typename Policy, typename T>
  SmartPtr<WeakPtr<typename std::remove_reference<T>::type::Type>,
    Policy>
  weak_ptr(T&& object) /*throw (eh::Exception)*/
  {
    return weak_ptr(object.in());
  }
}

#endif
