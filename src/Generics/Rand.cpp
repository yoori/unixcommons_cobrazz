// Generics/Rand.cpp
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstdint>
#include <unistd.h>

#include <Sync/PosixLock.hpp>

#include <Generics/ISAAC.hpp>
#include <Generics/MT19937.hpp>


namespace Generics
{
  namespace
  {
    Sync::PosixMutex mutex;
    ISAAC generator;
    std::atomic<uint64_t> unsafe_rand_seed_counter(0);

    inline uint64_t
    splitmix64(uint64_t& state) throw ()
    {
      uint64_t result = (state += 0x9E3779B97F4A7C15ULL);
      result = (result ^ (result >> 30)) * 0xBF58476D1CE4E5B9ULL;
      result = (result ^ (result >> 27)) * 0x94D049BB133111EBULL;
      return result ^ (result >> 31);
    }

    uint64_t
    unsafe_rand_seed() throw ()
    {
      uint64_t state = unsafe_rand_seed_counter.fetch_add(
        0x9E3779B97F4A7C15ULL,
        std::memory_order_relaxed);
      state ^= static_cast<uint64_t>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count());
      state ^= static_cast<uint64_t>(::getpid()) << 32;
      state ^= reinterpret_cast<uintptr_t>(&state);

      uint64_t seed = splitmix64(state);
      return seed ? seed : 0x9E3779B97F4A7C15ULL;
    }

    inline uint64_t
    xorshift64(uint64_t& state) throw ()
    {
      uint64_t result = state;
      result ^= result >> 12;
      result ^= result << 25;
      result ^= result >> 27;
      state = result;
      return result * 0x2545F4914F6CDD1DULL;
    }
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
    static thread_local uint64_t state = unsafe_rand_seed();
    return static_cast<uint32_t>(xorshift64(state) >> 33);
  }
}
