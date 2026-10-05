#include <new>

#define XXH_INLINE_ALL
#define XXH_VECTOR XXH_AVX512
#include <Generics/third_party/xxhash/xxhash.h>

#include <Generics/HashXxh3Backend.hpp>

namespace
{
  std::size_t hash(const void* key, std::size_t len, std::uint64_t seed) noexcept
  {
    return XXH3_64bits_withSeed(key, len, seed);
  }

  void update(void* storage, const void* key, std::size_t len) noexcept
  {
    auto* state = std::launder(reinterpret_cast<XXH3_state_t*>(storage));
    XXH3_64bits_update(state, key, len);
  }
}

namespace Generics::HashHelper
{
  const Xxh3Backend XXH3_AVX512 = {hash, update};
}
