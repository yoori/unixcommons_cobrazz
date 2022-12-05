#ifndef CHECKCOMMONS_COUNTER
#define CHECKCOMMONS_COUNTER

#include <sstream>

#include <eh/Exception.hpp>
#include <Generics/AtomicInt.hpp>

namespace TestCommons
{
  class Counter
  {
  public:
    Counter() throw ();

    void
    print() const /*throw (eh::Exception)*/;

    void
    print(std::ostream& ostr) const /*throw (eh::Exception)*/;

    void
    success() throw ();

    void
    failure() throw ();

    int
    succeeded() const throw ();

    int
    failed() const throw ();

  private:
    Generics::AtomicInt success_, failure_;
  };
}

#include "Counter.ipp"

#endif
