// Generics/Trace.hpp
#ifndef GENERICS_TRACE_HPP
#define GENERICS_TRACE_HPP

#ifdef BUILD_WITH_DEBUG_MESSAGES
#include <pthread.h>
#include <iostream>
#include <sstream>

#include <Generics/Time.hpp>


namespace
{
  template <typename Function, typename Param>
  inline
  void
  trace_message(Function fun, const Param& param) /*throw (eh::Exception)*/
  {
    Generics::Time tm = Generics::Time::get_time_of_day();
    std::ostringstream ostr;
    ostr << " [" << tm.get_local_time() << ",tid=" << pthread_self() << "]: "
         << fun << " " << param << std::endl;
    std::cout << ostr.str() << std::flush;
  }
}
#else
#define trace_message(x, y)
#endif

#endif
