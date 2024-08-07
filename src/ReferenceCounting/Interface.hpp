// file      : ReferenceCounting/Interface.hpp
// author    : Boris Kolpackov <boris@kolpackov.net>
// copyright : Copyright (c) 2002-2003 Boris Kolpackov
// license   : http://kolpackov.net/license.html

#ifndef REFERENCE_COUNTING_INTERFACE_HPP
#define REFERENCE_COUNTING_INTERFACE_HPP

#ifdef DEV_DEBUG
#include <valgrind/valgrind.h>
#else
#ifndef NVALGRIND
#define NVALGRIND
#endif
#endif

#include <Generics/Uncopyable.hpp>


namespace ReferenceCounting
{
  // Interface to a reference-countable object. Note that _remove_ref ()
  // member function has a no-throw semantic. Even though it can lead to
  // a diagnostic loss it was made no-throw because it has a destructor
  // semantic.

  class Interface
  {
  public:
    virtual
    void
    add_ref() const noexcept = 0;

    virtual
    void
    remove_ref() const noexcept = 0;

  protected:
    Interface() noexcept;
    Interface(const volatile Interface& iface) noexcept;

    virtual
    ~Interface() noexcept;

  private:
    void
    operator =(const volatile Interface& iface) noexcept;
  };

  template <typename Type>
  Type*
  add_ref(Type* ptr) noexcept;

#ifndef NVALGRIND
  template <const int C = 0>
  class RunningOnValgrind
  {
  public:
    template <typename RefCount>
    static
    void
    check_ref_count(const RefCount& ref_count) noexcept;

  private:
    static const int FLAG_;

    static
    int
    running_on_valgrind() noexcept;
  };
#endif
}

namespace ReferenceCounting
{
  //
  // Interface class
  //

  inline
  Interface::Interface() noexcept
  {
  }

  inline
  Interface::Interface(const volatile Interface& /*iface*/) noexcept
  {
  }

  inline
  Interface::~Interface() noexcept
  {
  }


  template <typename Type>
  inline
  Type*
  add_ref (Type* ptr) noexcept
  {
    if (ptr)
    {
      ptr->add_ref();
    }
    return ptr;
  }


#ifndef NVALGRIND
  //
  // RunningOnValgrind class
  //

  template <const int C>
  int
  RunningOnValgrind<C>::running_on_valgrind() noexcept
  {
    int flag = RUNNING_ON_VALGRIND;
    return flag;
  }

  template <const int C>
  template <typename RefCount>
  void
  RunningOnValgrind<C>::check_ref_count(const RefCount& ref_count) noexcept
  {
    if (ref_count && FLAG_)
    {
      new char;
    }
  }

  template <const int C>
  const int RunningOnValgrind<C>::FLAG_ = running_on_valgrind();
#endif
}

#endif
