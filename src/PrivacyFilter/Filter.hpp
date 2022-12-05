#ifndef PRIVACYFILTER_FILTER_HPP
#define PRIVACYFILTER_FILTER_HPP

#include <String/SubString.hpp>


namespace PrivacyFilter
{
  /**
   * Shows whether or not something should be filtered.
   * @return filtering status
   */
  bool
  filter() throw ();

  /**
   * Filters messages depending on found correct key.
   * @param original_message message to return if key is found
   * @param replace_message message to return if key is not found
   * @return original_message if key is found or replace_message otherwise
   */
  const char*
  filter(const char* original_message, const char* replace_message = "")
    throw ();

  /**
   * Filters messages depending on found correct key.
   * @param original_message message to return if key is found
   * @param replace_message message to return if key is not found
   * @return original_message if key is found or replace_message otherwise
   */
  const String::SubString&
  filter(const String::SubString& original_message,
    const String::SubString& replace_message)
    throw ();
}

#endif
