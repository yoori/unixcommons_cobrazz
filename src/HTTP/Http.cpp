#include <String/AsciiStringManip.hpp>

#include <Generics/BoolFunctors.hpp>

#include <HTTP/Http.hpp>


namespace
{
  // RFC 2616
  const String::AsciiStringManip::CharCategory LWS(" \t");
  const String::AsciiStringManip::CharCategory SEPARATORS(
    "()<>@,;:\\\"/[]?={} \t");

  const String::AsciiStringManip::CharCategory NAME(
    Generics::and1(
      String::AsciiStringManip::CharCategory(" -\x7E"),
      Generics::not1(SEPARATORS)));

  const String::AsciiStringManip::CharCategory VALUE("\t -\x7E");
}

namespace HTTP
{
  bool
  check_header(const char* name, const char* value) throw ()
  {
    if (!name || !value)
    {
      return false;
    }

    // Check name
    if (*NAME.find_nonowned(name))
    {
      return false;
    }

    // Check value
    for (const char* ptr = value; *(ptr = VALUE.find_nonowned(ptr));)
    {
      if (*ptr == '\r' && ptr[1] == '\n' && LWS(ptr[2]))
      {
        ptr += 2;
        continue;
      }
      return false;
    }
    return true;
  }

  bool
  check_headers(const HeaderList& headers) throw ()
  {
    for (HeaderList::const_iterator itor(headers.begin());
      itor != headers.end(); ++itor)
    {
      if (!check_header(itor->name.c_str(), itor->value.c_str()))
      {
        return false;
      }
    }
    return true;
  }
}
