#ifndef REFERENCECOUNTING_NULLPTR_HPP
#define REFERENCECOUNTING_NULLPTR_HPP


#if __GNUC__ == 4 && __GNUC_MINOR__ == 4
namespace ReferenceCounting
{
  struct NullPtr
  {
    template <typename T>
    operator T*() const throw ();
    template <typename T, typename D>
    operator D T::*() const throw ();
    operator bool() const throw ();
    bool
    operator ==(const NullPtr&) const throw ();
    bool
    operator !=(const NullPtr&) const throw ();

    NullPtr() throw () = delete;
    void
    operator &() throw () = delete;
    void
    operator =(NullPtr&) throw () = delete;
    void
    operator =(NullPtr&&) throw () = delete;
  };
}

static const ReferenceCounting::NullPtr nullptr(nullptr);

namespace std
{
  typedef decltype(nullptr) nullptr_t;

  template <typename T>
  typename add_rvalue_reference<T>::type
  declval() throw ();
}


namespace ReferenceCounting
{
  template <typename T>
  NullPtr::operator T*() const throw ()
  {
    return 0;
  }

  template <typename T, typename D>
  NullPtr::operator D T::*() const throw ()
  {
    return 0;
  }

  inline
  NullPtr::operator bool() const throw ()
  {
    return false;
  }

  inline
  bool
  NullPtr::operator ==(const NullPtr&) const throw ()
  {
    return true;
  }

  inline
  bool
  NullPtr::operator !=(const NullPtr&) const throw ()
  {
    return false;
  }
}
#endif

#endif
