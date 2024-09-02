#ifndef USERVER_FILEMANAGER_CONFIG_HPP
#define USERVER_FILEMANAGER_CONFIG_HPP

// POSIX
#include <liburing.h>

// STD
#include <cstdint>

namespace UServerUtils::FileManager
{

struct Config final
{
  std::uint32_t io_uring_size = 16384;
  std::uint32_t io_uring_flags = IORING_SETUP_ATTACH_WQ;
  std::uint32_t event_queue_max_size = 100000;
  std::uint32_t number_io_urings = 1;
};

} // namespace UServerUtils::FileManager

#endif // USERVER_FILEMANAGER_CONFIG_HPP