#pragma once

#include <HTTP/HttpAsync.hpp>
#include <HTTP/HttpAsyncPolicies.hpp>
#include <HTTP/HttpSync.hpp>
#include <HTTP/HttpClient.hpp>


namespace HTTP
{
  /**
   * Checks separate HTTP header for RFC compliance
   * @param name header name
   * @param value header value
   * @return whether or not header is RFC compliant
   */
  bool check_header(const char* name, const char* value) noexcept;

  /**
   * Checks headers for RFC compliance
   * @param headers list of headers
   * @return true only if every header is RFC compliant
   */
  bool check_headers(const HeaderList& headers) noexcept;
}
