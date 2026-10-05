#include <new>
#include <type_traits>

#define XXH_INLINE_ALL
#include <Generics/third_party/xxhash/xxhash.h>

#include <Generics/Hash.hpp>
#include <Generics/HashXxh3Backend.hpp>
#include <Generics/SimdCapabilities.hpp>

namespace
{
  XXH3_state_t* state(void* storage) noexcept
  {
    return std::launder(reinterpret_cast<XXH3_state_t*>(storage));
  }

  const XXH3_state_t* state(const void* storage) noexcept
  {
    return std::launder(reinterpret_cast<const XXH3_state_t*>(storage));
  }

  std::size_t hash(const void* key, std::size_t len, std::uint64_t seed) noexcept
  {
    return XXH3_64bits_withSeed(key, len, seed);
  }

  void update(void* storage, const void* key, std::size_t len) noexcept
  {
    XXH3_64bits_update(state(storage), key, len);
  }

  const Generics::HashHelper::Xxh3Backend& backend() noexcept
  {
    static const auto* selected = []() noexcept
    {
#ifdef GENERICS_XXH3_HAS_AVX512
      if (Generics::Simd::has(Generics::Simd::AVX512F | Generics::Simd::AVX512BW))
      {
        return &Generics::HashHelper::XXH3_AVX512;
      }
#endif
#ifdef GENERICS_XXH3_HAS_AVX2
      if (Generics::Simd::has(Generics::Simd::AVX2))
      {
        return &Generics::HashHelper::XXH3_AVX2;
      }
#endif
      return &Generics::HashHelper::XXH3_BASELINE;
    }();
    return *selected;
  }
}

namespace Generics::HashHelper
{
  const Xxh3Backend XXH3_BASELINE = {hash, update};
}

namespace Generics
{
  XXH3Hasher::XXH3Hasher(Calc seed) noexcept
  {
    static_assert(sizeof(XXH3_state_t) == sizeof(state_));
    static_assert(alignof(XXH3_state_t) <= alignof(XXH3Hasher));
    static_assert(std::is_trivially_destructible_v<XXH3_state_t>);
    auto* initialized = new (state_) XXH3_state_t;
    XXH3_INITSTATE(initialized);
    XXH3_64bits_reset_withSeed(initialized, seed);
  }

  XXH3Hasher::XXH3Hasher(const XXH3Hasher& other) noexcept
  {
    new (state_) XXH3_state_t;
    XXH3_copyState(state(state_), state(other.state_));
  }

  XXH3Hasher& XXH3Hasher::operator=(const XXH3Hasher& other) noexcept
  {
    if (this != &other)
    {
      XXH3_copyState(state(state_), state(other.state_));
    }
    return *this;
  }

  void XXH3Hasher::add(const void* key, std::size_t len) noexcept
  {
    if (len != 0)
    {
      backend().update(state_, key, len);
    }
  }

  std::size_t XXH3Hasher::finalize() const noexcept
  {
    return XXH3_64bits_digest(state(state_));
  }

  std::size_t XXH3Hasher::hash(const void* key, std::size_t len, Calc seed) noexcept
  {
    // The short-input algorithm is scalar and needs no SIMD dispatch.
    return len <= 240 ? XXH3_64bits_withSeed(key, len, seed) : backend().hash(key, len, seed);
  }
}
