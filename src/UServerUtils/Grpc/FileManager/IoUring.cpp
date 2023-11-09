// POSIX
#include <ctype.h>
#include <errno.h>
#include <sys/utsname.h>

// STD
#include <cstring>
#include <iostream>
#include <sstream>

// THIS
#include <Generics/Function.hpp>
#include <UServerUtils/Grpc/FileManager/IoUring.hpp>
#include <UServerUtils/Grpc/FileManager/Utils.hpp>

namespace UServerUtils::Grpc::FileManager
{

IoUring::IoUring(
  const Config& config,
  const std::optional<UringFd> uring_fd)
{
  std::memset(&params_, 0, sizeof(params_));
  params_.flags |= config.io_uring_flags;

  if ((params_.flags & IORING_SETUP_ATTACH_WQ) != 0)
  {
    if (!uring_fd.has_value())
    {
      std::ostringstream stream;
      stream << FNS
             << "with flag IORING_SETUP_ATTACH_WQ uring_fd must be set";
      throw Exception(stream.str());
    }

    params_.wq_fd = *uring_fd;
  }

  const auto result = io_uring_queue_init_params(
    config.io_uring_size,
    &ring_,
    &params_);
 if (result)
  {
    const auto error = -result;
    std::ostringstream stream;
    stream << FNS;
    if (error == 12)
    {
      const auto kernel_version = linux_kernel_version();
      if (kernel_version.size() >= 2
       && kernel_version[0] <= 5
       && kernel_version[1] < 12)
      {
        // https://github.com/axboe/liburing/issues/246
        stream << "For 5.11 and down kernel version require "
                  "rlimit memlock allocations. ";
      }
    }

    stream << "[code="
           << error
           << ", message="
           << Utils::safe_strerror(error)
           << "]";
    throw Exception(stream.str());
  }
}

IoUring::Version IoUring::linux_kernel_version() const
{
  Version version;
  version.reserve(16);

  utsname buffer;
  Utils::ScopedErrno scoped_errno;
  if (uname(&buffer) < 0)
  {
    std::ostringstream stream;
    stream << FNS
           << ": uname is failed, reason=[code="
           << errno
           << ", message="
           << Utils::safe_strerror(scoped_errno.get())
           << "]";
    throw Exception(stream.str());
  }

  char* pointer = buffer.release;
  while (*pointer)
  {
    if (std::isdigit(*pointer))
    {
      version.emplace_back(
        std::strtol(pointer, &pointer, 10));
    }
    else
    {
      pointer += 1;
    }
  }

  return version;
}

IoUring::~IoUring()
{
  io_uring_queue_exit(&ring_);
}

const io_uring_params& IoUring::params() const noexcept
{
  return params_;
}

io_uring* IoUring::get() noexcept
{
  return &ring_;
}

} // namespace UServerUtils::Grpc::FileManager