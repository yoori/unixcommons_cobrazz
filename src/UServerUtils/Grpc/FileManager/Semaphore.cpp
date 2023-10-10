// STD
#include <sstream>

// POSIX
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

// THIS
#include <Generics/Function.hpp>
#include <UServerUtils/Grpc/FileManager/Semaphore.hpp>
#include <UServerUtils/Grpc/FileManager/Utils.hpp>

namespace UServerUtils::Grpc::FileManager
{

Semaphore::Semaphore(
  const bool is_nonblocking,
  const std::uint32_t init_value)
{
  Utils::ScopedErrno scoped_errno;
  int flags = EFD_SEMAPHORE;
  if (is_nonblocking)
  {
    flags |= O_NONBLOCK;
  }

  descriptor_ = eventfd(init_value, flags);
  if (descriptor_ == -1)
  {
    std::ostringstream stream;
    stream << FNS
           << "eventfd is failed, reason=[code="
           << scoped_errno.get()
           << ", message="
           << Utils::safe_strerror(scoped_errno.get())
           << "]";
    throw Exception(stream.str());
  }
}

Semaphore::~Semaphore()
{
  close(descriptor_);
}

int Semaphore::fd() const noexcept
{
  return descriptor_;
}

bool Semaphore::add() const noexcept
{
  const std::uint64_t value = 1;
  const auto result = eventfd_write(descriptor_, value);
  return result == 0;
}

bool Semaphore::fetch() const noexcept
{
  std::uint64_t value = 0;
  const auto result = eventfd_read(descriptor_, &value);
  return result == 0;
}

} // namespace UServerUtils::Grpc::FileManager