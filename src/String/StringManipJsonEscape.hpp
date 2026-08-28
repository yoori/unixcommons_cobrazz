#pragma once

#include <cstdint>
#include <string>

#include <String/SubString.hpp>

namespace String::StringManip::JsonEscape
{
  enum class SimdLevel
  {
    AUTO,
    SCALAR,
    SSE2,
    AVX2,
    AVX512BW
  };

  bool simd_level_available(SimdLevel level) noexcept;

  SimdLevel default_simd_level() noexcept;

  const char* simd_level_name(SimdLevel level) noexcept;

  void
  json_escape_append(
    std::string& dest,
    const String::SubString& src,
    SimdLevel simd_level) /*throw (eh::Exception)*/;

  inline bool is_non_json_char(char ch) noexcept
  {
    return static_cast<unsigned char>(ch) < 0x20 || ch == '"' || ch == '\\';
  }

  inline const char* find_non_json_scalar(const char* cur, const char* end) noexcept
  {
    for (; cur != end; ++cur)
    {
      if (is_non_json_char(*cur))
      {
        return cur;
      }
    }

    return end;
  }

  const char* find_non_json_sse2(const char* cur, const char* end) noexcept;

#if defined(STRING_MANIP_JSON_ESCAPE_HAS_AVX2)
  const char* find_non_json_avx2(const char* cur, const char* end) noexcept;
#endif

#if defined(STRING_MANIP_JSON_ESCAPE_HAS_AVX512BW)
  const char* find_non_json_avx512bw(const char* cur, const char* end) noexcept;
#endif
}
