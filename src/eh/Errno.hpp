#pragma once

#include <errno.h>

#include <String/StringManip.hpp>


//
// Errno throw helper
//

namespace eh::ErrnoHelper
{
  // There are two versions of strerror_r functions with
  // different behavior
  inline const char* error_message(const char* /*buf*/, const char* function_result)
  {
    return function_result;
  }

  inline const char* error_message(const char* buf, int /*function_result*/)
  {
    return buf;
  }

  template <typename... Args>
  void compose_safe(char* string, size_t string_size, int error, Args... args) noexcept
  {
    char error_buf[128];
    char buf[128];

    String::StringManip::int_to_str(error, error_buf, sizeof(error_buf));
    String::StringManip::concat(string, string_size, args...,
      ": errno = ", error_buf, ": ",
      error_message(buf, strerror_r(error, buf, sizeof(buf))));
  }
}

namespace eh
{
  template <typename SomeException, typename... Args>
  void throw_errno_exception(int error, Args... args) /*throw (SomeException)*/
  {
    char string[sizeof(SomeException)];
    ErrnoHelper::compose_safe(string, sizeof(string), error, args...);
    throw SomeException(string);
  }

  template <typename SomeException, typename... Args>
  void throw_errno_exception(Args... args) /*throw (SomeException)*/
  {
    throw_errno_exception<SomeException>(errno, args...);
  }
}
