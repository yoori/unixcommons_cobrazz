#include <charconv>
#include <cstring>

#include <Generics/HashTableAdapters.hpp>

namespace std
{
  size_t
  to_chars_len(const Generics::StringHashAdapter& sha) noexcept
  {
    return sha.text().size();
  }

  std::to_chars_result
  to_chars(char* first, char* last, const Generics::StringHashAdapter& sha)
    noexcept
  {
    size_t capacity = last - first;
    if (sha.text().size() > capacity)
    {
      return {last, std::errc::value_too_large};
    }
    memcpy(first, sha.text().data(), sha.text().size());
    return {first + sha.text().size(), std::errc()};
  }

  std::string
  to_string(const Generics::StringHashAdapter& sha) noexcept
  {
    return sha.text();
  }
}
