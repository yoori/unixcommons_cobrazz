#pragma once

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
    void add(const String::SubString& error, bool write = false) noexcept;

    void print() const noexcept;

    void print(std::ostream& ostr) const noexcept;

    bool empty() const noexcept;

  private:
    using AllErrors = std::map<std::string, int>;

    mutable Sync::PosixMutex mutex_;
    AllErrors errors_;
  };
}

//
// Inlines
//

namespace TestCommons
{
  inline bool Errors::empty() const noexcept
  {
    Sync::PosixGuard guard(mutex_);

    return errors_.empty();
  }
}
