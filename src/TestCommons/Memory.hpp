#ifndef TESTCOMMONS_MEMORY_HPP
#define TESTCOMMONS_MEMORY_HPP

#include <iostream>

#include <malloc.h>

#include <eh/Exception.hpp>


namespace TestCommons
{
  /**
   * Prints detailed information from mallinfo structure into the
   * specified stream.
   * @param ostr stream to use for output
   * @param info structure to print. If 0 - retrieve current information
   * automatically before printing.
   */
  void
  print_mallinfo(std::ostream& ostr, struct mallinfo* info = 0)
    /*throw (eh::Exception)*/;
}

#endif
