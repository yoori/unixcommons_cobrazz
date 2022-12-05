#include "Korean.hpp"

namespace Language
{
  namespace Segmentor
  {
    namespace Korean
    {
      const char HANGUL_RANGES[] =
        "\xE1\x84\x80-\xE1\x87\xB9" // U+1100-U+11F9
        "\xE3\x80\xAE-\xE3\x80\xAF" // U+302E-U+302F
        "\xE3\x84\xB1-\xE3\x86\x8E" // U+3131-U+318E
        "\xE3\x88\x80-\xE3\x88\x9E" // U+3200-U+321E
        "\xE3\x89\xA0-\xE3\x89\xBF" // U+3260-U+327F
        "\xEA\xB0\x80-\xED\x9E\xA3" // U+AC00-U+D7A3
        "\xEF\xBE\xA0-\xEF\xBF\x9C" // U+FFA0-U+FFDC
        ;

      NotHangul NOT_HANGUL(HANGUL_RANGES);
    }
  }
}

