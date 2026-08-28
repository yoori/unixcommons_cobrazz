#pragma once

#include <sstream>

#include <eh/Exception.hpp>
#include <Generics/AtomicInt.hpp>

namespace TestCommons
{
  class Counter
  {
  public:
    Counter() noexcept;

    void print() const /*throw (eh::Exception)*/;

    void print(std::ostream& ostr) const /*throw (eh::Exception)*/;

    void success() noexcept;

    void failure() noexcept;

    int succeeded() const noexcept;

    int failed() const noexcept;

  private:
    Generics::AtomicInt success_, failure_;
  };
}

#include "Counter.ipp"
