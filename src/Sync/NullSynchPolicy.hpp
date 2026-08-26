// file      : Utility/Synch/Policy/Null.hpp
// author    : Boris Kolpackov <boris@kolpackov.net>
// copyright : Copyright (c) 2002-2003 Boris Kolpackov
// license   : http://kolpackov.net/license.html

#pragma once

#include <Generics/Uncopyable.hpp>


namespace Sync
{
  namespace Policy
  {
    class NullMutex : private Generics::Uncopyable
    {
    };

    class NullGuard : private Generics::Uncopyable
    {
    public:
      explicit
      NullGuard(NullMutex&) noexcept;
    };

    struct Null
    {
      typedef NullMutex Mutex;
      typedef NullGuard ReadGuard;
      typedef NullGuard WriteGuard;
    };
  }
}


namespace Sync
{
  namespace Policy
  {
    inline
    NullGuard::NullGuard (NullMutex&) noexcept
    {
    }
  }
}
