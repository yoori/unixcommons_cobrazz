#pragma once

#include <String/StringManip.hpp>
#include <String/UTF8Handler.hpp>
#include <String/UTF8Category.hpp>


namespace Language
{
  namespace Segmentor
  {
    namespace Korean
    {
      typedef const String::StringManip::InverseCategory<
        String::Utf8Category> NotHangul;
      extern NotHangul NOT_HANGUL;
    }
  }
}
