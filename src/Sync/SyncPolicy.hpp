#pragma once

#include <Sync/PosixLock.hpp>


namespace Sync
{
  namespace Policy
  {
    template <typename AdoptedMutex, typename AdoptedReadGuard,
      typename AdoptedWriteGuard>
    class PolicyAdapter
    {
    public:
      typedef AdoptedMutex Mutex;
      typedef AdoptedReadGuard ReadGuard;
      typedef AdoptedWriteGuard WriteGuard;
    };

    using PosixThread = PolicyAdapter<::Sync::PosixMutex, ::Sync::PosixGuard, ::Sync::PosixGuard>;
    using PosixSpinThread = PolicyAdapter<
      ::Sync::PosixSpinLock, ::Sync::PosixSpinGuard, ::Sync::PosixSpinGuard>;
    using PosixThreadRW =
      PolicyAdapter<::Sync::PosixRWLock, ::Sync::PosixRGuard, ::Sync::PosixWGuard>;
  }
}
