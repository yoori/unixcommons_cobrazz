#include <iostream>
#include <string_view>

#include <String/UTF8Handler.hpp>

namespace
{
  bool expect_valid(std::string_view value, const char* name)
  {
    if (!String::UTF8Handler::is_correct_utf8_string(value))
    {
      return true;
    }

    std::cerr << name << ": valid string rejected" << std::endl;
    return false;
  }

  bool expect_invalid(std::string_view value, std::size_t offset, const char* name)
  {
    const char* const invalid = String::UTF8Handler::is_correct_utf8_string(value);
    if (invalid == value.data() + offset)
    {
      return true;
    }

    std::cerr << name << ": invalid string accepted or wrong offset returned" << std::endl;
    return false;
  }
}

int main()
{
  const char valid_with_zero[] = {'a', '\0', static_cast<char>(0xD0), static_cast<char>(0xAF)};
  const char invalid_after_zero[] = {'a', '\0', static_cast<char>(0xC0)};
  const char truncated[] = {'a', static_cast<char>(0xE2), static_cast<char>(0x82)};
  const char surrogate[] = {
    'a', static_cast<char>(0xED), static_cast<char>(0xA0), static_cast<char>(0x80)};

  bool result = true;
  result &= expect_valid({}, "empty");
  result &= expect_valid(
    std::string_view(valid_with_zero, sizeof(valid_with_zero)), "embedded zero");
  result &= expect_invalid(
    std::string_view(invalid_after_zero, sizeof(invalid_after_zero)), 2, "invalid after zero");
  result &= expect_invalid(std::string_view(truncated, sizeof(truncated)), 1, "truncated");
  result &= expect_invalid(std::string_view(surrogate, sizeof(surrogate)), 1, "surrogate");
  return result ? 0 : 1;
}
