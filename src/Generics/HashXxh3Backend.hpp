#pragma once

#include <cstddef>
#include <cstdint>

namespace Generics::HashHelper
{
  struct Xxh3Backend
  {
    std::size_t (*hash)(const void*, std::size_t, std::uint64_t) noexcept;
    void (*update)(void*, const void*, std::size_t) noexcept;
  };

  extern const Xxh3Backend XXH3_BASELINE;
#ifdef GENERICS_XXH3_HAS_AVX2
  extern const Xxh3Backend XXH3_AVX2;
#endif
#ifdef GENERICS_XXH3_HAS_AVX512
  extern const Xxh3Backend XXH3_AVX512;
#endif
}
