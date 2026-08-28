#pragma once

#include <Sync/PosixLock.hpp>


namespace Sync::Policy
{
  template <typename AdoptedMutex, typename AdoptedReadGuard, typename AdoptedWriteGuard>
  class PolicyAdapter
  {
  public:
    using Mutex = AdoptedMutex;
    using ReadGuard = AdoptedReadGuard;
    using WriteGuard = AdoptedWriteGuard;
  };

  using PosixThread = PolicyAdapter<::Sync::PosixMutex, ::Sync::PosixGuard, ::Sync::PosixGuard>;
  using PosixSpinThread = PolicyAdapter<
    ::Sync::PosixSpinLock, ::Sync::PosixSpinGuard, ::Sync::PosixSpinGuard>;
  using PosixThreadRW =
    PolicyAdapter<::Sync::PosixRWLock, ::Sync::PosixRGuard, ::Sync::PosixWGuard>;
}
