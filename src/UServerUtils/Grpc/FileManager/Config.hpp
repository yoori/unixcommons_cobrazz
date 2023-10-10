#ifndef USERVER_FILEMANAGER_CONFIG_HPP
#define USERVER_FILEMANAGER_CONFIG_HPP

// STD
#include <cstdint>

namespace UServerUtils::Grpc::FileManager
{

/**
 * Important flags(io_uring_flags):
 * 1. IORING_SETUP_IOPOLL - ensure that the underlying device has
 *   polling configured (see poll_queues=X for nvme)
 * 2. IORING_SETUP_ATTACH_WQ - the io_uring instance being created
 *    will share the asynchronous worker thread backend  of the
 *    specified io_uring ring, rather than create a new separate
 *    thread pool.
 * Example: io_uring_flags = IORING_SETUP_IOPOLL | IORING_SETUP_ATTACH_WQ
 *
 * If you open file with O_DIRECT the source of the data(buffer)
 * being written must be memory-aligne.
 **/
struct Config final
{
  Config() = default;
  ~Config() = default;

  std::uint32_t io_uring_size = 16384;
  std::uint32_t io_uring_flags = 0;
  std::uint32_t event_queue_max_size = 10000;
  std::uint32_t number_io_urings = 1;
};

} // namespace UServerUtils::Grpc::FileManager

#endif // USERVER_FILEMANAGER_CONFIG_HPP
