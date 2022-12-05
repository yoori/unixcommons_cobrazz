#ifndef GENERICS_ATOMIC_HPP_
#define GENERICS_ATOMIC_HPP_

#if __GNUC__ >= 8
#  include <atomic>
#  define USE_STD_ATOMIC
#else
#  if __GNUC__ > 4 || __GNUC__ == 4 && __GNUC_MINOR__ >= 2
#    include <ext/atomicity.h>
#  else
#    include <bits/atomicity.h>
#  endif
#endif

#include <stdint.h>
#include <limits>

#ifdef USE_STD_ATOMIC
  typedef int AtomicHolderInternalType;
  typedef std::atomic<int> AtomicHolder;
#else
  typedef _Atomic_word AtomicHolderInternalType;
  typedef _Atomic_word AtomicHolder;
#endif

namespace Generics
{
  class AtomicInt
  {
  public:
    AtomicInt(int val);

    int exchange_and_add(int val);

    AtomicInt& operator+=(int val);

    AtomicInt& operator-=(int val);

    int
    operator++();

    int
    operator++(int);

    int
    operator--();

    int
    operator--(int);

    operator int() const;

  private:
    AtomicInt(const AtomicInt&);

  private:
    volatile AtomicHolder value_;
  };

  class AtomicUInt
  {
  public:
    AtomicUInt(unsigned int val);

    unsigned int
    exchange_and_add(int val);

    AtomicUInt& operator+=(int val);

    AtomicUInt& operator-=(int val);

    unsigned int
    operator++();

    unsigned int
    operator++(int);

    unsigned int
    operator--();

    unsigned int
    operator--(int);

    operator unsigned int() const;

  protected:
    int
    exchange_and_add_(int val);

    static unsigned int
    get_value_(int val);

  private:
    volatile AtomicHolder value_;
  };
}

namespace Generics
{
  // AtomicInt
  inline
  AtomicInt::AtomicInt(int val)
    : value_(val)
  {}

  inline
  int
  AtomicInt::exchange_and_add(int val)
  {
#ifdef USE_STD_ATOMIC
    return value_.fetch_add(val);
#else
    return __gnu_cxx::__exchange_and_add(&value_, val);
#endif
  }

  inline
  AtomicInt&
  AtomicInt::operator+=(int val)
  {
#ifdef USE_STD_ATOMIC
    value_ += val;
#else
    __gnu_cxx::__atomic_add(&value_, val);
#endif
    return *this;
  }

  inline
  AtomicInt&
  AtomicInt::operator-=(int val)
  {
    *this += -val;
    return *this;
  }

  inline
  int
  AtomicInt::operator++()
  {
    return exchange_and_add(1) + 1;
  }

  inline
  int
  AtomicInt::operator++(int)
  {
    return exchange_and_add(1);
  }

  inline
  int
  AtomicInt::operator--()
  {
    return exchange_and_add(-1) - 1;
  }

  inline
  int
  AtomicInt::operator--(int)
  {
    return exchange_and_add(-1);
  }

  inline
  AtomicInt::operator int() const
  {
    return value_;
  }

  // AtomicUInt
  inline
  AtomicUInt::AtomicUInt(unsigned int val)
    : value_(
      static_cast<int64_t>(val) +
      std::numeric_limits<AtomicHolderInternalType>::min())
  {}

  inline
  unsigned int
  AtomicUInt::exchange_and_add(int val)
  {
#ifdef USE_STD_ATOMIC
    return get_value_(value_.fetch_add(val));
#else
    return get_value_(__gnu_cxx::__exchange_and_add(&value_, val));
#endif
  }

  inline
  AtomicUInt&
  AtomicUInt::operator+=(int val)
  {
#ifdef USE_STD_ATOMIC
    value_ += val;
#else
    __gnu_cxx::__atomic_add(&value_, val);
#endif
    return *this;
  }

  inline
  AtomicUInt&
  AtomicUInt::operator-=(int val)
  {
    *this += -val;
    return *this;
  }

  inline
  unsigned int
  AtomicUInt::operator++()
  {
    return get_value_(exchange_and_add_(1) + 1);
  }

  inline
  unsigned int
  AtomicUInt::operator++(int)
  {
    return get_value_(exchange_and_add_(1));
  }

  inline
  unsigned int
  AtomicUInt::operator--()
  {
    return get_value_(exchange_and_add_(-1) - 1);
  }

  inline
  unsigned int
  AtomicUInt::operator--(int)
  {
    return get_value_(exchange_and_add_(-1));
  }

  inline
  AtomicUInt::operator unsigned int() const
  {
    return get_value_(
      static_cast<int64_t>(value_) -
      std::numeric_limits<AtomicHolderInternalType>::min());
  }

  inline
  int
  AtomicUInt::exchange_and_add_(int val)
  {
#ifdef USE_STD_ATOMIC
    return value_.fetch_add(val);
#else
    return __gnu_cxx::__exchange_and_add(&value_, val);
#endif
  }

  inline
  unsigned int
  AtomicUInt::get_value_(int val)
  {
    return static_cast<int64_t>(val) -
      std::numeric_limits<AtomicHolderInternalType>::min();
  }
}

#endif /*GENERICS_ATOMIC_HPP_*/
