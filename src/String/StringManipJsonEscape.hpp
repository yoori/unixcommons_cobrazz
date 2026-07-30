#ifndef STRING_STRING_MANIP_JSON_ESCAPE_HPP
#define STRING_STRING_MANIP_JSON_ESCAPE_HPP

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

  bool
  simd_level_available(SimdLevel level) throw ();

  SimdLevel
  default_simd_level() throw ();

  const char*
  simd_level_name(SimdLevel level) throw ();

  void
  json_escape_append(
    std::string& dest,
    const String::SubString& src,
    SimdLevel simd_level) /*throw (eh::Exception)*/;

  inline
  bool
  is_non_json_char(char ch) throw ()
  {
    return static_cast<unsigned char>(ch) < 0x20 || ch == '"' || ch == '\\';
  }

  inline
  const char*
  find_non_json_scalar(const char* cur, const char* end) throw ()
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

  const char*
  find_non_json_sse2(const char* cur, const char* end) throw ();

#if defined(STRING_MANIP_JSON_ESCAPE_HAS_AVX2)
  const char*
  find_non_json_avx2(const char* cur, const char* end) throw ();
#endif

#if defined(STRING_MANIP_JSON_ESCAPE_HAS_AVX512BW)
  const char*
  find_non_json_avx512bw(const char* cur, const char* end) throw ();
#endif
}

#endif
