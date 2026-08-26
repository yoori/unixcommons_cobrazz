#include <Generics/SimdCapabilities.hpp>

namespace
{
  std::uint32_t
  detect_cpu_capabilities_() noexcept
  {
    std::uint32_t capabilities = 0;

#if defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
    __builtin_cpu_init();

    if (__builtin_cpu_supports("sse2"))
    {
      capabilities |= Generics::Simd::SSE2;
    }

    if (__builtin_cpu_supports("avx2"))
    {
      capabilities |= Generics::Simd::AVX2;
    }

    if (__builtin_cpu_supports("avx512f"))
    {
      capabilities |= Generics::Simd::AVX512F;
    }

    if (__builtin_cpu_supports("avx512bw"))
    {
      capabilities |= Generics::Simd::AVX512BW;
    }
#else
#if defined(__SSE2__)
    capabilities |= Generics::Simd::SSE2;
#endif

#if defined(__AVX2__)
    capabilities |= Generics::Simd::AVX2;
#endif

#if defined(__AVX512F__)
    capabilities |= Generics::Simd::AVX512F;
#endif

#if defined(__AVX512BW__)
    capabilities |= Generics::Simd::AVX512BW;
#endif
#endif

    return capabilities;
  }
}

namespace Generics::Simd
{
  extern const std::uint32_t CPU_CAPABILITIES = detect_cpu_capabilities_();
}
