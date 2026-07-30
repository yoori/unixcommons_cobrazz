#include <immintrin.h>

#include <String/StringManipJsonEscape.hpp>

namespace String::StringManip::JsonEscape
{
  const char*
  find_non_json_avx2(const char* cur, const char* end) throw ()
  {
    if (end - cur >= 32)
    {
      const __m256i quote = _mm256_set1_epi8('"');
      const __m256i backslash = _mm256_set1_epi8('\\');
      const __m256i control_max = _mm256_set1_epi8(0x1F);
      const __m256i zero = _mm256_setzero_si256();

      do
      {
        const __m256i value =
          _mm256_loadu_si256(reinterpret_cast<const __m256i*>(cur));
        const __m256i quote_mask = _mm256_cmpeq_epi8(value, quote);
        const __m256i backslash_mask = _mm256_cmpeq_epi8(value, backslash);
        const __m256i control_mask =
          _mm256_cmpeq_epi8(_mm256_subs_epu8(value, control_max), zero);
        const __m256i mask = _mm256_or_si256(
          _mm256_or_si256(quote_mask, backslash_mask),
          control_mask);
        const unsigned int bits =
          static_cast<unsigned int>(_mm256_movemask_epi8(mask));

        if (bits != 0)
        {
          return cur + __builtin_ctz(bits);
        }

        cur += 32;
      }
      while (end - cur >= 32);
    }

    return find_non_json_scalar(cur, end);
  }
}
