#include <charconv>

#include <Generics/HashTableAdapters.hpp>

namespace std 
{
  std::to_chars_result
  to_chars(char*, char* last, const Generics::StringHashAdapter&) /*throw (eh::Exception)*/
  {
    return {last, std::errc()}; 
  }

  std::string
  to_string(const Generics::StringHashAdapter&) /*throw (eh::Exception) */
  {
    return "";
  }
}


