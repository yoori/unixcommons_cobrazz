#pragma once

#include <String/StringManip.hpp>
#include <String/UTF8Handler.hpp>
#include <String/UTF8Category.hpp>


namespace Language::Segmentor
{
  namespace Korean
  {
    using NotHangul = const String::StringManip::InverseCategory<
      String::Utf8Category>;
    extern NotHangul NOT_HANGUL;
  }
}
