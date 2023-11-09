#ifndef USERVER_FILEMANAGER_CONFIG_HPP
#define USERVER_FILEMANAGER_CONFIG_HPP

// POSIX
#include <liburing.h>

// STD
#include <cstdint>

namespace UServerUtils::Grpc::FileManager
{

/**
 * Important flags(io_uring_flags):
 * 1. IORING_SETUP_IOPOLL - ensure that the underlying device has
 *   polling configured (see poll_queues=X for nvme). You must open
 *   file with flag O_DIRECT.
 * 2. IORING_SETUP_ATTACH_WQ - the io_uring instance being created
 *    will share the asynchronous worker thread backend  of the
 *    specified io_uring ring, rather than create a new separate
 *    thread pool.
 * Example: io_uring_flags = IORING_SETUP_IOPOLL | IORING_SETUP_ATTACH_WQ
 *
 * If you open file with O_DIRECT the source of the data(buffer)
 * being written must be memory-aligne. O_DIRECT is used to make sure
 * that reads and writes are not cached, and come straight from the
 * storage device. O_DIRECT has special requirements on read/write buffers
 * alignment, which are unspecified in "man open(2)". However a page-aligned
 * buffer works with linux filesystems (512 bytes is usually enough).
 **/
struct Config final
{
  Config() = default;
  ~Config() = default;

  std::uint32_t io_uring_size = 16384;
  std::uint32_t io_uring_flags = IORING_SETUP_ATTACH_WQ;
  std::uint32_t event_queue_max_size = 10000;
  std::uint32_t number_io_urings = 1;
};

} // namespace UServerUtils::Grpc::FileManager

#endif // USERVER_FILEMANAGER_CONFIG_HPP