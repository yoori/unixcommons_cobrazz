// file      : Utility/ReferenceCounting/SmartPtr.tpp
// author    : Boris Kolpackov <boris@kolpackov.net>
// copyright : Copyright (c) 2002-2003 Boris Kolpackov
// license   : http://kolpackov.net/license.html

#include <utility>


namespace ReferenceCounting
{
  //
  // in() and retn() functions for smart pointers
  //

  template <typename T, typename Policy>
  inline
  T*
  SmartPtr<T, Policy>::in() const noexcept
  {
    return ptr_;
  }

  template <typename T, typename Policy>
  T*
  SmartPtr<T, Policy>::retn() noexcept
  {
    Type* ret(ptr_);
    ptr_ = 0;
    return ret;
  }

  template <typename T, typename Policy>
  inline
  T*
  FixedPtr<T, Policy>::in() noexcept
  {
    return ptr_;
  }

  template <typename T, typename Policy>
  inline
  const T*
  FixedPtr<T, Policy>::in() const noexcept
  {
    return ptr_;
  }

  template <typename T, typename Policy>
  typename FixedPtr<T, Policy>::Type*
  FixedPtr<T, Policy>::retn() noexcept
  {
    Type* ret(ptr_);
    ptr_ = 0;
    return ret;
  }


  inline
  std::nullptr_t
  add_ref(std::nullptr_t ptr) noexcept
  {
    return ptr;
  }

  // Specialization of add_ref function for SmartPtr
  template <typename T, typename Policy>
  T*
  add_ref(const SmartPtr<T, Policy>& ptr) noexcept
  {
    return add_ref(ptr.in());
  }

  template <typename T, typename Policy>
  T*
  add_ref(SmartPtr<T, Policy>&& ptr) noexcept
  {
    return ptr.retn();
  }

  // Specializations of add_ref function for FixedPtr/QualPtr
  template <typename T, typename Policy>
  const T*
  add_ref(const FixedPtr<T, Policy>& ptr) noexcept
  {
    return add_ref(ptr.in());
  }

  template <typename T, typename Policy>
  T*
  add_ref(FixedPtr<T, Policy>& ptr) noexcept
  {
    return add_ref(ptr.in());
  }

  template <typename T, typename Policy>
  T*
  add_ref(FixedPtr<T, Policy>&& ptr) noexcept
  {
    return ptr.retn();
  }


  template <typename A>
  A*
  cond_add_ref(A* a) noexcept
  {
    return a;
  }

  template <typename A>
  auto
  //cond_add_ref(A&& a) noexcept -> decltype(add_ref(std::forward<A>(a)))
  cond_add_ref(A&& a) noexcept -> decltype(add_ref(static_cast<A&&>(a)))
  {
    return add_ref(std::forward<A>(a));
  }


  //
  // SmartPtr class
  //

  template <typename T, typename Policy>
  SmartPtr<T, Policy>::SmartPtr(std::nullptr_t ptr) noexcept
    : ptr_(ptr)
  {
    Policy::default_constructor();
  }

  template <typename T, typename Policy>
  SmartPtr<T, Policy>::SmartPtr(const SmartPtr& sptr) /*throw (NullPointer)*/
    : ptr_(add_ref(sptr))
  {
    Policy::check_init(ptr_);
  }

  template <typename T, typename Policy>
  SmartPtr<T, Policy>::SmartPtr(SmartPtr&& sptr) /*throw (NullPointer)*/
    : ptr_(sptr.retn())
  {
    Policy::check_init(ptr_);
  }

  template <typename T, typename Policy>
  template <typename Other>
  SmartPtr<T, Policy>::SmartPtr(Other&& sptr) /*throw (NullPointer)*/
    : ptr_(cond_add_ref(std::forward<Other>(sptr)))
  {
    Policy::check_init(ptr_);
  }


  template <typename T, typename Policy>
  SmartPtr<T, Policy>::~SmartPtr() noexcept
  {
    static_assert(is_correct_policy<Policy>, "Try use unknown policy for SmartPtr");
    if (ptr_)
    {
      ptr_->remove_ref();
    }
  }


  template <typename T, typename Policy>
  template <typename Other>
  SmartPtr<T, Policy>&
  SmartPtr<T, Policy>::operator =(Other&& sptr) /*throw (NullPointer)*/
  {
    T* new_ptr = cond_add_ref(std::forward<Other>(sptr));
    if (ptr_)
    {
      ptr_->remove_ref();
    }
    ptr_ = new_ptr;
    Policy::check_init(ptr_);
    return *this;
  }

  template <typename T, typename Policy>
  SmartPtr<T, Policy>&
  SmartPtr<T, Policy>::operator =(const SmartPtr& sptr)
    /*throw (NullPointer)*/
  {
    return operator =(add_ref(sptr));
  }


  template <typename T, typename Policy>
  template <typename OtherPolicy>
  void
  SmartPtr<T, Policy>::swap(SmartPtr<Type, OtherPolicy>& sptr)
    /*throw (NullPointer, typename OtherPolicy::NullPointer)*/
  {
    Type* new_ptr(sptr.in());
    Policy::check_init(new_ptr);
    OtherPolicy::check_init(ptr_);
    sptr.retn();
    sptr = ptr_;
    ptr_ = new_ptr;
  }


  template <typename T, typename Policy>
  void
  SmartPtr<T, Policy>::reset() /*throw (NullPointer)*/
  {
    if (ptr_)
    {
      ptr_->remove_ref();
    }
    ptr_ = 0;
    Policy::check_init(ptr_);
  }


  template <typename T, typename Policy>
  inline
  SmartPtr<T, Policy>::operator T*() const noexcept
  {
    return ptr_;
  }


  template <typename T, typename Policy>
  inline
  T*
  SmartPtr<T, Policy>::operator ->() const /*throw (NotInitialized)*/
  {
    Policy::check_dereference(ptr_);
    return ptr_;
  }

  template <typename T, typename Policy>
  inline
  T&
  SmartPtr<T, Policy>::operator *() const /*throw (NotInitialized)*/
  {
    return *operator ->();
  }


  //
  // FixedPtr class
  //

  template <typename T, typename Policy>
  FixedPtr<T, Policy>::FixedPtr() noexcept
    : ptr_(0)
  {
    Policy::default_constructor();
  }

  template <typename T, typename Policy>
  FixedPtr<T, Policy>::FixedPtr(
    typename Generics::IfConst<T, const FixedPtr, FixedPtr>::
      Result& sptr) /*throw (NullPointer)*/
    : Generics::Uncopyable(), ptr_(add_ref(sptr))
  {
    Policy::check_init(ptr_);
  }

  template <typename T, typename Policy>
  FixedPtr<T, Policy>::FixedPtr(FixedPtr&& sptr)
    /*throw (NullPointer)*/
    : Generics::Uncopyable(), ptr_(sptr.retn())
  {
    Policy::check_init(ptr_);
  }

  template <typename T, typename Policy>
  template <typename Other>
  FixedPtr<T, Policy>::FixedPtr(Other&& sptr)
    /*throw (NullPointer)*/
    : Generics::Uncopyable(), ptr_(cond_add_ref(std::forward<Other>(sptr)))
  {
    Policy::check_init(ptr_);
  }

  template <typename T, typename Policy>
  FixedPtr<T, Policy>::~FixedPtr() noexcept
  {
    static_assert(is_correct_policy<Policy>, "Try use unknown policy for FixedPtr");
    if (ptr_)
    {
      ptr_->remove_ref();
    }
  }


  template <typename T, typename Policy>
  inline
  FixedPtr<T, Policy>::operator T*() noexcept
  {
    return ptr_;
  }

  template <typename T, typename Policy>
  inline
  FixedPtr<T, Policy>::operator const T*() const noexcept
  {
    return ptr_;
  }

  template <typename T, typename Policy>
  inline
  T*
  FixedPtr<T, Policy>::operator ->() /*throw (NotInitialized)*/
  {
    Policy::check_dereference(ptr_);
    return ptr_;
  }

  template <typename T, typename Policy>
  inline
  const T*
  FixedPtr<T, Policy>::operator ->() const /*throw (NotInitialized)*/
  {
    Policy::check_dereference(ptr_);
    return ptr_;
  }

  template <typename T, typename Policy>
  inline
  T&
  FixedPtr<T, Policy>::operator *() /*throw (NotInitialized)*/
  {
    return *operator ->();
  }

  template <typename T, typename Policy>
  inline
  const T&
  FixedPtr<T, Policy>::operator *() const /*throw (NotInitialized)*/
  {
    return *operator ->();
  }


  //
  // QualPtr class
  //

  template <typename T, typename Policy>
  QualPtr<T, Policy>::QualPtr() /*throw (NullPointer)*/
  {
  }

  /*
  template <typename T, typename Policy>
  QualPtr<T, Policy>::QualPtr(const QualPtr<T, Policy>& init)
    : Base(cond_add_ref(init))
  {}
  */

  template <typename T, typename Policy>
  QualPtr<T, Policy>::QualPtr(QualPtr<T, Policy>&& init)
    : Base(std::forward<QualPtr<T, Policy> >(init))
  {}

  template <typename T, typename Policy>
  template <typename Other>
  QualPtr<T, Policy>::QualPtr(Other&& sptr) /*throw (NullPointer)*/
    : Base(std::forward<Other>(sptr))
  {
  }


  template <typename T, typename Policy>
  template <typename Other>
  QualPtr<T, Policy>&
  QualPtr<T, Policy>::operator =(Other&& sptr) /*throw (NullPointer)*/
  {
    T* new_ptr = cond_add_ref(std::forward<Other>(sptr));
    if (ptr_)
    {
      ptr_->remove_ref();
    }
    ptr_ = new_ptr;
    Policy::check_init(ptr_);
    return *this;
  }

  template <typename T, typename Policy>
  QualPtr<T, Policy>&
  QualPtr<T, Policy>::operator =(
    typename Generics::IfConst<T, const QualPtr, QualPtr>::
      Result& sptr) /*throw (NullPointer)*/
  {
    return operator =(add_ref(sptr));
  }

  template <typename T, typename Policy>
  template <typename OtherPolicy>
  void
  QualPtr<T, Policy>::swap(QualPtr<Type, OtherPolicy>& sptr)
    /*throw (NullPointer, typename OtherPolicy::NullPointer)*/
  {
    Type* new_ptr(sptr.in());
    Policy::check_init(new_ptr);
    OtherPolicy::check_init(ptr_);
    sptr.retn();
    sptr = ptr_;
    ptr_ = new_ptr;
  }


  template <typename T, typename Policy>
  void
  QualPtr<T, Policy>::reset() /*throw (NullPointer)*/
  {
    if (ptr_)
    {
      ptr_->remove_ref();
    }
    ptr_ = 0;
    Policy::check_init(ptr_);
  }


  //
  // ConstPtr class
  //

  template <typename T, typename Policy>
  ConstPtr<T, Policy>::ConstPtr() /*throw (NullPointer)*/
  {
  }

  template <typename T, typename Policy>
  ConstPtr<T, Policy>::ConstPtr(const ConstPtr& sptr) /*throw (NullPointer)*/
    : Base(sptr)
  {
  }

  template <typename T, typename Policy>
  template <typename Other>
  ConstPtr<T, Policy>::ConstPtr(Other&& sptr) /*throw (NullPointer)*/
    : Base(std::forward<Other>(sptr))
  {
  }


  template <typename T, typename Policy>
  ConstPtr<T, Policy>&
  ConstPtr<T, Policy>::operator =(const ConstPtr& sptr)
    /*throw (NullPointer)*/
  {
    Base::operator =(sptr);
    return *this;
  }

  //
  // make_ref
  //

  template<typename T, typename Policy, typename... Args>
  SmartPtr<T, Policy>
  make_ref(Args&&... args) /*throw (eh::Exception)*/
  {
    return SmartPtr<T, Policy>(new T(std::forward<Args>(args)...));
  }

  template<typename T, typename Policy = PolicyThrow, typename... Args>
  FixedPtr<T, Policy>
  make_ref_fixed(Args&&... args) /*throw (eh::Exception)*/
  {
    return FixedPtr<T, Policy>(new T(std::forward<Args>(args)...));
  }

  template<typename T, typename Policy = PolicyThrow, typename... Args>
  QualPtr<T, Policy>
  make_ref_qual(Args&&... args) /*throw (eh::Exception)*/
  {
    return QualPtr<T, Policy>(new T(std::forward<Args>(args)...));
  }

  template<typename T, typename Policy = PolicyThrow, typename... Args>
  ConstPtr<T, Policy>
  make_ref_const(Args&&... args) /*throw (eh::Exception)*/
  {
    return ConstPtr<T, Policy>(new T(std::forward<Args>(args)...));
  }
}
