#pragma once

#include <cstdint>

namespace Generics::Simd
{
  enum Capability : std::uint32_t
  {
    SSE2 = 1u << 0,
    AVX2 = 1u << 1,
    AVX512F = 1u << 2,
    AVX512BW = 1u << 3
  };

  extern const std::uint32_t CPU_CAPABILITIES;

  inline
  bool
  has(std::uint32_t capabilities) noexcept
  {
    return (CPU_CAPABILITIES & capabilities) == capabilities;
  }
}
