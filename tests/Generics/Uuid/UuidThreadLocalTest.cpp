#include <algorithm>
#include <atomic>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>

#include <openssl/rand.h>

#include <Generics/Uuid.hpp>

namespace
{
  std::atomic<unsigned> seed_calls{0};

  void check(bool condition, const char* message)
  {
    if (!condition)
    {
      throw std::runtime_error(message);
    }
  }

  void test_thread_seeds()
  {
    constexpr unsigned threads = 8;
    constexpr unsigned waves = 2;
    constexpr unsigned count = 10000;
    const auto initial_calls = seed_calls.load();
    std::vector<Generics::Uuid> all;
    all.reserve(threads * waves * count + 2);
    all.push_back(Generics::Uuid::create_random_based());
    all.push_back(Generics::Uuid::create_random_based());
    check(seed_calls.load() == initial_calls + 1, "Main thread was not seeded exactly once");

    for (unsigned wave = 0; wave < waves; ++wave)
    {
      std::vector<std::vector<Generics::Uuid>> batches(threads);
      std::vector<std::thread> workers;
      std::atomic<unsigned> ready{0};
      std::atomic<bool> go{false};
      for (unsigned slot = 0; slot < threads; ++slot)
      {
        batches[slot].reserve(count);
        workers.emplace_back([&, slot]
          {
            ready.fetch_add(1, std::memory_order_relaxed);
            while (!go.load(std::memory_order_acquire))
            {
              std::this_thread::yield();
            }

            for (unsigned i = 0; i < count; ++i)
            {
              batches[slot].push_back(Generics::Uuid::create_random_based());
            }
          });
      }

      while (ready.load(std::memory_order_relaxed) != threads)
      {
        std::this_thread::yield();
      }

      go.store(true, std::memory_order_release);
      for (auto& worker : workers)
      {
        worker.join();
      }

      for (const auto& batch : batches)
      {
        all.insert(all.end(), batch.begin(), batch.end());
      }
    }

    check(seed_calls.load() == initial_calls + 1 + threads * waves,
      "Each thread must initialize its generator exactly once");
    for (const auto& uuid : all)
    {
      check((uuid.begin()[6] & 0xf0) == 0x40, "Invalid UUID version");
      check((uuid.begin()[8] & 0xc0) == 0x80, "Invalid UUID variant");
    }

    std::sort(all.begin(), all.end());
    check(std::adjacent_find(all.begin(), all.end()) == all.end(),
      "Threads reused a UUID sequence");
  }
}

// Fixed entropy exposes seed reuse, including when a new thread reuses an old thread's storage.
extern "C" int RAND_bytes(unsigned char* buffer, int size)
{
  std::fill_n(buffer, size, 0xa5);
  seed_calls.fetch_add(1, std::memory_order_relaxed);
  return 1;
}

int main()
{
  try
  {
    test_thread_seeds();
    std::cout << "TestUuidThreadLocal: PASS (160002 UUIDs, 17 generator seeds)\n";
    return 0;
  }
  catch (const std::exception& error)
  {
    std::cerr << "TestUuidThreadLocal: " << error.what() << '\n';
    return 1;
  }
}
