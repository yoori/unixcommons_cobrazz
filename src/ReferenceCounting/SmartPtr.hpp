// file      : ReferenceCounting/SmartPtr.hpp
// author    : Boris Kolpackov <boris@kolpackov.net>
// copyright : Copyright (c) 2002-2003 Boris Kolpackov
// license   : http://kolpackov.net/license.html

#ifndef REFERENCE_COUNTING_SMART_PTR_HPP
#define REFERENCE_COUNTING_SMART_PTR_HPP

#include <eh/Exception.hpp>

#include <ReferenceCounting/Interface.hpp>
#include <ReferenceCounting/NullPtr.hpp>

#include <Generics/TypeTraits.hpp>


namespace ReferenceCounting
{
  /**
   * Default policy of SmartPtr. SmartPtr can hold any value and the
   * check (and exception throwing) is performed on dereference
   */
  class PolicyThrow
  {
  public:
    typedef eh::Exception NullPointer;
    DECLARE_EXCEPTION(NotInitialized, eh::DescriptiveException);

    static
    void
    check_init(const void* ptr) /*throw (NullPointer)*/
      __attribute__((always_inline));

    static
    void
    check_dereference(const void* ptr) /*throw (NotInitialized)*/
      __attribute__((always_inline));

    static
    void
    default_constructor() noexcept
      __attribute__((always_inline));

    static
    void
    retn() noexcept
      __attribute__((always_inline));

  private:
    ~PolicyThrow() noexcept
      __attribute__((always_inline));
  };

  /**
   * Assert policy of SmartPtr. SmartPtr can hold any value and the
   * check (via assert) is performed on dereference
   */
  class PolicyAssert
  {
  public:
    typedef eh::Exception NullPointer;
    typedef eh::Exception NotInitialized;

    static
    void
    check_init(const void* ptr) /*throw (NullPointer)*/
      __attribute__((always_inline));

    static
    void
    check_dereference(const void* ptr) /*throw (NotInitialized)*/
      __attribute__((always_inline));

    static
    void
    default_constructor() noexcept
      __attribute__((always_inline));

    static
    void
    retn() noexcept
      __attribute__((always_inline));

  private:
    ~PolicyAssert() noexcept
      __attribute__((always_inline));
  };

  /**
   * Non-null policy of SmartPtr. SmartPtr cannot have null pointer stored.
   * Thus default constructor and retn() are inapplicable.
   */
  class PolicyNotNull
  {
  public:
    DECLARE_EXCEPTION(NullPointer, eh::DescriptiveException);
    typedef eh::Exception NotInitialized;

    static
    void
    check_init(const void* ptr) /*throw (NullPointer)*/
      __attribute__((always_inline));

    static
    void
    check_dereference(const void* ptr) /*throw (NotInitialized)*/
      __attribute__((always_inline));

  private:
    ~PolicyNotNull() noexcept
      __attribute__((always_inline));
  };

  struct PolicyChecker
  {
    void
    check_policy_(const PolicyThrow* policy) noexcept
      __attribute__((always_inline));
    void
    check_policy_(const PolicyAssert* policy) noexcept
      __attribute__((always_inline));
    void
    check_policy_(const PolicyNotNull* policy) noexcept
      __attribute__((always_inline));
  };

  template <typename T, typename Policy>
  class SmartPtr;
  template <typename T, typename Policy>
  class FixedPtr;
  template <typename T, typename Policy>
  class QualPtr;
  template <typename T, typename Policy>
  class ConstPtr;


  std::nullptr_t
  add_ref(std::nullptr_t ptr) noexcept;

  template <typename T, typename Policy>
  T*
  add_ref(const SmartPtr<T, Policy>& ptr) noexcept;

  template <typename T, typename Policy>
  T*
  add_ref(SmartPtr<T, Policy>&& ptr) noexcept;

  template <typename T, typename Policy>
  const T*
  add_ref(const FixedPtr<T, Policy>& ptr) noexcept;

  template <typename T, typename Policy>
  T*
  add_ref(FixedPtr<T, Policy>& ptr) noexcept;

  template <typename T, typename Policy>
  T*
  add_ref(FixedPtr<T, Policy>&& ptr) noexcept;


  template <typename T, typename Policy = PolicyThrow>
  class SmartPtr : private PolicyChecker
  {
  public:
    typedef T Type;

    typedef typename Policy::NullPointer NullPointer;
    typedef typename Policy::NotInitialized NotInitialized;

  public:
    // c-tors
    SmartPtr(std::nullptr_t ptr = nullptr) noexcept;

    SmartPtr(const SmartPtr& sptr) /*throw (NullPointer)*/;

    SmartPtr(SmartPtr&& sptr) /*throw (NullPointer)*/;

    template <typename Other>
    SmartPtr(Other&& sptr) /*throw (NullPointer)*/;

    // d-tor
    ~SmartPtr() noexcept;

    // assignment & copy-assignment operators
    SmartPtr&
    operator =(const SmartPtr& sptr) /*throw (NullPointer)*/;

    template <typename Other>
    SmartPtr&
    operator =(Other&& sptr) /*throw (NullPointer)*/;

    template <typename OtherPolicy>
    void
    swap(SmartPtr<Type, OtherPolicy>& sptr)
      /*throw (NullPointer, typename OtherPolicy::NullPointer)*/;

    void
    reset() /*throw (NullPointer)*/;

    // conversions
    operator Type*() const noexcept
      __attribute__((always_inline));

    // accessors
    Type*
    operator ->() const /*throw (NotInitialized)*/
      __attribute__((always_inline));

    Type&
    operator *() const /*throw (NotInitialized)*/
      __attribute__((always_inline));

    Type*
    in() const noexcept
      __attribute__((always_inline));

    Type*
    retn() noexcept;

  private:
    Type* ptr_;
  };

  template <typename T, typename Policy = PolicyThrow>
  class FixedPtr :
    private PolicyChecker,
    private Generics::Uncopyable
  {
  public:
    typedef T Type;

    typedef typename Policy::NullPointer NullPointer;
    typedef typename Policy::NotInitialized NotInitialized;

  public:
    // c-tors
    FixedPtr(typename Generics::IfConst<T, const FixedPtr, FixedPtr>::
      Result& sptr) /*throw (NullPointer)*/;

    FixedPtr(FixedPtr&& sptr) /*throw (NullPointer)*/;

    FixedPtr(typename Generics::IfConst<T, int, const FixedPtr&>::Result) =
      delete;

    template <typename Other>
    FixedPtr(Other&& sptr) /*throw (NullPointer)*/;

    // d-tor
    ~FixedPtr() noexcept;

    // conversions
    operator Type*() noexcept
      __attribute__((always_inline));

    operator const Type*() const noexcept
      __attribute__((always_inline));

    // accessors
    Type*
    operator ->() /*throw (NotInitialized)*/
      __attribute__((always_inline));

    const Type*
    operator ->() const /*throw (NotInitialized)*/
      __attribute__((always_inline));

    Type&
    operator *() /*throw (NotInitialized)*/
      __attribute__((always_inline));

    const Type&
    operator *() const /*throw (NotInitialized)*/
      __attribute__((always_inline));

    Type*
    in() noexcept
      __attribute__((always_inline));

    const Type*
    in() const noexcept
      __attribute__((always_inline));

  protected:
    FixedPtr() noexcept;

    Type*
    retn() noexcept;

    Type* ptr_;

    friend
    Type*
    add_ref<T, Policy>(FixedPtr&& ptr) noexcept;
  };

  template <typename T, typename Policy = PolicyThrow>
  class QualPtr : public FixedPtr<T, Policy>
  {
  public:
    typedef FixedPtr<T, Policy> Base;

    typedef typename Base::Type Type;

    typedef typename Base::NullPointer NullPointer;
    typedef typename Base::NotInitialized NotInitialized;

  public:
    // c-tors
    QualPtr() /*throw (NullPointer)*/;

    //QualPtr(const QualPtr<T, Policy>& init);

    QualPtr(QualPtr<T, Policy>&& init);

    template <typename Other>
    QualPtr(Other&& sptr) /*throw (NullPointer)*/;

    // assignment & copy-assignment operators
    QualPtr&
    operator =(typename Generics::IfConst<T, const QualPtr, QualPtr>::
      Result& sptr) /*throw (NullPointer)*/;

    QualPtr&
    operator =(typename Generics::IfConst<T, int, const QualPtr&>::Result) =
      delete;

    template <typename Other>
    QualPtr&
    operator =(Other&& sptr) /*throw (NullPointer)*/;

    template <typename OtherPolicy>
    void
    swap(QualPtr<Type, OtherPolicy>& sptr)
      /*throw (NullPointer, typename OtherPolicy::NullPointer)*/;

    void
    reset() /*throw (NullPointer)*/;

    using Base::retn;

  private:
    using Base::ptr_;
  };

  template <typename T, typename Policy = PolicyThrow>
  class ConstPtr : public QualPtr<const T, Policy>
  {
  public:
    typedef QualPtr<const T, Policy> Base;

    typedef typename Base::Type Type;

    typedef typename Base::NullPointer NullPointer;
    typedef typename Base::NotInitialized NotInitialized;

  public:
    // c-tors
    ConstPtr() /*throw (NullPointer)*/;

    ConstPtr(const ConstPtr& sptr) /*throw (NullPointer)*/;

    template <typename Other>
    ConstPtr(Other&& sptr) /*throw (NullPointer)*/;

    // assignment & copy-assignment operators
    using Base::operator =;

    ConstPtr&
    operator =(const ConstPtr& sptr) /*throw (NullPointer)*/;
  };


  template <typename T>
  struct ThrowPtr
  {
    typedef SmartPtr<T, PolicyThrow> Ptr;
    typedef FixedPtr<T, PolicyThrow> FPtr;
    typedef QualPtr<T, PolicyThrow> QPtr;
  };

  template <typename T>
  struct AssertPtr
  {
    typedef SmartPtr<T, PolicyAssert> Ptr;
    typedef FixedPtr<T, PolicyAssert> FPtr;
    typedef QualPtr<T, PolicyAssert> QPtr;
  };

  template <typename T>
  struct NonNullPtr
  {
    typedef SmartPtr<T, PolicyNotNull> Ptr;
    typedef FixedPtr<T, PolicyNotNull> FPtr;
    typedef QualPtr<T, PolicyNotNull> QPtr;
  };

  //
  // make_ref (like make_unique or make_shared)
  //

  template<typename T, typename Policy = PolicyThrow, typename... Args>
  SmartPtr<T, Policy>
  make_ref(Args&&... args) /*throw (eh::Exception)*/;

  template<typename T, typename Policy = PolicyThrow, typename... Args>
  FixedPtr<T, Policy>
  make_ref_fixed(Args&&... args) /*throw (eh::Exception)*/;

  template<typename T, typename Policy = PolicyThrow, typename... Args>
  QualPtr<T, Policy>
  make_ref_qual(Args&&... args) /*throw (eh::Exception)*/;

  template<typename T, typename Policy = PolicyThrow, typename... Args>
  ConstPtr<T, Policy>
  make_ref_const(Args&&... args) /*throw (eh::Exception)*/;
}

#include <ReferenceCounting/SmartPtr.ipp>
#include <ReferenceCounting/SmartPtr.tpp>

#endif
