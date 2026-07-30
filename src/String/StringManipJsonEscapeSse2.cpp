#include <emmintrin.h>

#include <String/StringManipJsonEscape.hpp>

namespace String::StringManip::JsonEscape
{
  const char*
  find_non_json_sse2(const char* cur, const char* end) throw ()
  {
    if (end - cur >= 16)
    {
      const __m128i quote = _mm_set1_epi8('"');
      const __m128i backslash = _mm_set1_epi8('\\');
      const __m128i control_max = _mm_set1_epi8(0x1F);
      const __m128i zero = _mm_setzero_si128();

      do
      {
        const __m128i value =
          _mm_loadu_si128(reinterpret_cast<const __m128i*>(cur));
        const __m128i quote_mask = _mm_cmpeq_epi8(value, quote);
        const __m128i backslash_mask = _mm_cmpeq_epi8(value, backslash);
        const __m128i control_mask =
          _mm_cmpeq_epi8(_mm_subs_epu8(value, control_max), zero);
        const __m128i mask = _mm_or_si128(
          _mm_or_si128(quote_mask, backslash_mask),
          control_mask);
        const unsigned int bits =
          static_cast<unsigned int>(_mm_movemask_epi8(mask));

        if (bits != 0)
        {
          return cur + __builtin_ctz(bits);
        }

        cur += 16;
      }
      while (end - cur >= 16);
    }

    return find_non_json_scalar(cur, end);
  }
}
