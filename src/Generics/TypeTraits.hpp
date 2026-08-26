// Generics/TypeTraits.hpp
#pragma once

#include <limits>

#include <Generics/Uncopyable.hpp>


namespace Generics
{
  template <typename T, typename A, typename B>
  struct IfConst
  {
    typedef B Result;
  };
  template <typename T, typename A, typename B>
  struct IfConst<const T, A, B>
  {
    typedef A Result;
  };

  template <typename Container>
  class Inserter : private Generics::Uncopyable
  {
  public:
    explicit
    Inserter(Container& container) noexcept;
    Inserter&
    operator *() noexcept;
    template <typename T>
    Inserter&
    operator =(T&& data) /*throw (eh::Exception)*/;

  private:
    Container& container_;
  };

  template <typename Integer>
  Integer
  safe_next(Integer number) noexcept
  {
    return number < std::numeric_limits<Integer>::max() ?
      number + 1 : number;
  }

  template <typename From, typename To>
  class PairPtr : private Generics::Uncopyable
  {
  public:
    PairPtr(From* from) noexcept;
    PairPtr(PairPtr&& other) noexcept;

    To*
    operator ->() noexcept;

  private:
    To to;
  };
}

namespace Generics
{
  //
  // Inserter class
  //

  template <typename Container>
  Inserter<Container>::Inserter(Container& container) noexcept
    : container_(container)
  {
  }

  template <typename Container>
  Inserter<Container>&
  Inserter<Container>::operator *() noexcept
  {
    return *this;
  }

  template <typename Container>
  template <typename T>
  Inserter<Container>&
  Inserter<Container>::operator =(T&& data) /*throw (eh::Exception)*/
  {
    container_.insert(std::forward<T>(data));
    return *this;
  }


  //
  // PairPtr class
  //

  template <typename From, typename To>
  PairPtr<From, To>::PairPtr(From* from) noexcept
    : to(from->first, from->second)
  {
  }

  template <typename From, typename To>
  PairPtr<From, To>::PairPtr(PairPtr&& other) noexcept
    : to(other.to)
  {
  }

  template <typename From, typename To>
  To*
  PairPtr<From, To>::operator ->() noexcept
  {
    return &to;
  }
}
