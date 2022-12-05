/**
 * @file   SyncPolicy.hpp
 * @author Karen Aroutiounov
 */

#ifndef SYNC_SYNC_POLICY_HPP
#define SYNC_SYNC_POLICY_HPP

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

    typedef PolicyAdapter<PosixMutex, PosixGuard, PosixGuard>
      PosixThread;
    typedef PolicyAdapter<PosixSpinLock, PosixSpinGuard, PosixSpinGuard>
      PosixSpinThread;
    typedef PolicyAdapter<PosixRWLock, PosixRGuard, PosixWGuard>
      PosixThreadRW;
  }
}

#endif
