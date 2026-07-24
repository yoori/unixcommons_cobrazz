// Generics/Rand.cpp
#include <cstdlib>

#include <Sync/PosixLock.hpp>

#include <Generics/ISAAC.hpp>
#include <Generics/MT19937.hpp>


namespace Generics
{
  namespace
  {
    Sync::PosixMutex mutex;
    ISAAC generator;
  }

  const size_t MT19937::STATE_SIZE;
  const uint32_t MT19937::RAND_MAXIMUM;

  const uint32_t ISAAC::RAND_MAXIMUM;
  const size_t ISAAC::SIZE;

  uint32_t
  safe_rand() throw ()
  {
    Sync::PosixGuard lock(mutex);
    return generator.rand() >> 1;
  }

  uint32_t
  unsafe_rand() throw ()
  {
    return static_cast<uint32_t>(std::rand());
  }
}
