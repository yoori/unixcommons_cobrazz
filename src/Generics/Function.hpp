#ifndef GENERICS_FUNCTION_HPP
#define GENERICS_FUNCTION_HPP

#include <String/SubString.hpp>


namespace Generics
{
  namespace FunctionHelper
  {
    inline
    String::SubString
    get_function_name(const char* function) throw ()
    {
      for (const char* end = function;; end++)
      {
        switch (*end)
        {
        case ' ':
          if (end != function && end[-1] != ',')
          {
            function = end + 1;
          }
          continue;
        case '(':
        case '\0':
          break;
        default:
          continue;
        }
        return String::SubString(function, end);
      }
    }

    inline
    String::SubString
    get_template_info(const char* function) throw ()
    {
      const char with[] = " [with ";
      for (; *function; function++)
      {
        size_t i = 0;
        for (; i < sizeof(with) - 1; i++)
        {
          if (function[i] != with[i])
          {
            break;
          }
        }
        if (i == sizeof(with) - 1)
        {
          break;
        }
      }
      if (!*function)
      {
        return String::SubString();
      }
      function += sizeof(with) - 1;
      return String::SubString(function,
        String::CharTraits<char>::length(function) - 1);
    }
  }
}

/**
 * Base function name. Useful only when a single parameter should be
 * passed to a function (trace_message, for example)
 */
#define FNB Generics::FunctionHelper::get_function_name(__PRETTY_FUNCTION__)
/**
 * Stream function name. Useful with Stream::Error or other stream usage.
 */
#define FNS FNB << "(): "
/**
 * Stream function name with template information.
 * Useful with Stream::Error or other stream usage.
 */
#define FNT FNB << "<" << \
  Generics::FunctionHelper::get_template_info(__PRETTY_FUNCTION__) << \
  ">(): "
/**
 * Designed specially for eh::throw_errno_exception function.
 */
#define FNE FNB, "(): "

#endif
