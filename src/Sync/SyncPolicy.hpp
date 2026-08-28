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

    typedef PolicyAdapter<::Sync::PosixMutex, ::Sync::PosixGuard, ::Sync::PosixGuard>
      PosixThread;
    typedef PolicyAdapter<::Sync::PosixSpinLock, ::Sync::PosixSpinGuard, ::Sync::PosixSpinGuard>
      PosixSpinThread;
    typedef PolicyAdapter<::Sync::PosixRWLock, ::Sync::PosixRGuard, ::Sync::PosixWGuard>
      PosixThreadRW;
  }
}
