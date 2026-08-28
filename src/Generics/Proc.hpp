#pragma once

#include <cstddef>


namespace Generics::Proc
{
  /**
   * Reads process' memory information from proc(5)
   * @param vsize virtual memory size
   * @param rss resident memory size
   * @return whether or not reading has been successful
   */
  bool memory_status(unsigned long& vsize, unsigned long& rss) noexcept;

  /**
   * Fills buffer with backtrace information
   * @param buf buffer to fill
   * @param size its size
   * @param from the deepest function to add
   * @param to the most shallow function to add
   */
  void backtrace(char* buf, size_t size, size_t from, size_t to) noexcept;
}
