#include <immintrin.h>

#include <String/StringManipJsonEscape.hpp>

namespace String::StringManip::JsonEscape
{
  const char* find_non_json_avx512bw(const char* cur, const char* end) noexcept
  {
    if (end - cur >= 64)
    {
      const __m512i quote = _mm512_set1_epi8('"');
      const __m512i backslash = _mm512_set1_epi8('\\');
      const __m512i control_max = _mm512_set1_epi8(0x1F);
      const __m512i zero = _mm512_setzero_si512();

      do
      {
        const __m512i value = _mm512_loadu_si512(cur);
        const __mmask64 bits = _mm512_cmpeq_epi8_mask(value, quote) |
          _mm512_cmpeq_epi8_mask(value, backslash) |
          _mm512_cmpeq_epi8_mask( _mm512_subs_epu8(value, control_max), zero);

        if (bits != 0)
        {
          return cur + __builtin_ctzll(bits);
        }

        cur += 64;
      }
      while (end - cur >= 64);
    }

    return find_non_json_scalar(cur, end);
  }
}
