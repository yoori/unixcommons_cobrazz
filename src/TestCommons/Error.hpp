#ifndef CHECKCOMMONS_ERROR
#define CHECKCOMMONS_ERROR

#include <iostream>
#include <string>
#include <map>

#include <String/SubString.hpp>
#include <Sync/PosixLock.hpp>

namespace TestCommons
{
  class Errors
  {
  public:
    void
    add(const String::SubString& error, bool write = false) throw ();

    void
    print() const throw ();

    void
    print(std::ostream& ostr) const throw ();

    bool
    empty() const throw ();

  private:
    typedef std::map<std::string, int> AllErrors;

    mutable Sync::PosixMutex mutex_;
    AllErrors errors_;
  };
}

//
// Inlines
//

namespace TestCommons
{
  inline
  bool
  Errors::empty() const throw ()
  {
    Sync::PosixGuard guard(mutex_);

    return errors_.empty();
  }
}

#endif
