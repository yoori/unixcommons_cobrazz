#ifndef REFERENCECOUNTING_NULLPTR_HPP
#define REFERENCECOUNTING_NULLPTR_HPP


#if __GNUC__ == 4 && __GNUC_MINOR__ == 4
namespace ReferenceCounting
{
  struct NullPtr
  {
    template <typename T>
    operator T*() const noexcept;
    template <typename T, typename D>
    operator D T::*() const noexcept;
    operator bool() const noexcept;
    bool
    operator ==(const NullPtr&) const noexcept;
    bool
    operator !=(const NullPtr&) const noexcept;

    NullPtr() noexcept = delete;
    void
    operator &() noexcept = delete;
    void
    operator =(NullPtr&) noexcept = delete;
    void
    operator =(NullPtr&&) noexcept = delete;
  };
}

static const ReferenceCounting::NullPtr nullptr(nullptr);

namespace std
{
  typedef decltype(nullptr) nullptr_t;

  template <typename T>
  typename add_rvalue_reference<T>::type
  declval() noexcept;
}


namespace ReferenceCounting
{
  template <typename T>
  NullPtr::operator T*() const noexcept
  {
    return 0;
  }

  template <typename T, typename D>
  NullPtr::operator D T::*() const noexcept
  {
    return 0;
  }

  inline
  NullPtr::operator bool() const noexcept
  {
    return false;
  }

  inline
  bool
  NullPtr::operator ==(const NullPtr&) const noexcept
  {
    return true;
  }

  inline
  bool
  NullPtr::operator !=(const NullPtr&) const noexcept
  {
    return false;
  }
}
#endif

#endif
