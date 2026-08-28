// file      : Utility/Synch/Policy/Null.hpp
// author    : Boris Kolpackov <boris@kolpackov.net>
// copyright : Copyright (c) 2002-2003 Boris Kolpackov
// license   : http://kolpackov.net/license.html

#pragma once

#include <Generics/Uncopyable.hpp>


namespace Sync::Policy
{
  class NullMutex : private Generics::Uncopyable
  {
  };

  class NullGuard : private Generics::Uncopyable
  {
  public:
    explicit NullGuard(NullMutex&) noexcept;
  };

  struct Null
  {
    using Mutex = NullMutex;
    using ReadGuard = NullGuard;
    using WriteGuard = NullGuard;
  };
}


namespace Sync::Policy
{
  inline NullGuard::NullGuard (NullMutex&) noexcept
  {
  }
}
