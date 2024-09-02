// STD
#include <sstream>

// POSIX
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

// THIS
#include <Generics/Function.hpp>
#include <UServerUtils/FileManager/Utils.hpp>

namespace UServerUtils::FileManager
{

inline Semaphore::Semaphore(
  const Type type,
  const std::uint32_t init_value)
  : type_(type)
{
  Utils::ScopedErrno scoped_errno;
  int flags = EFD_SEMAPHORE;
  if (type == Type::NonBlocking)
  {
    flags |= O_NONBLOCK;
  }

  fd_ = eventfd(init_value, flags);
  if (fd_ == -1)
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

inline Semaphore::~Semaphore()
{
  close(fd_);
}

inline int Semaphore::fd() const noexcept
{
  return fd_;
}

inline bool Semaphore::add() const noexcept
{
  const std::uint64_t value = 1;
  const auto result = eventfd_write(fd_, value);
  return result == 0;
}

inline bool Semaphore::consume() const noexcept
{
  std::uint64_t value = 0;
  const auto result = eventfd_read(fd_, &value);
  return result == 0;
}

inline std::uint32_t Semaphore::try_consume(const std::uint32_t v) const noexcept
{
  std::uint32_t result = 0;
  if (type_ == Type::NonBlocking)
  {
    while (result < v && consume())
    {
      result += 1;
    }
  }
  else
  {
    NonBlockingScope scope(fd_);
    while (result < v && consume())
    {
      result += 1;
    }
  }

  return result;
}

inline Semaphore::NonBlockingScope::NonBlockingScope(const int fd) noexcept
  : fd_(fd)
{
  const int result = add_flags(fd_, O_NONBLOCK);
  assert(result >= 0);
  if (result >= 0)
  {
    old_flags_ = result;
  }
}

inline Semaphore::NonBlockingScope::~NonBlockingScope()
{
  if (old_flags_ >= 0)
  {
    const int result = set_flags(fd_, old_flags_);
    assert(result == 0);
  }
}

inline int Semaphore::NonBlockingScope::add_flags(
  const int fd,
  const int flags) const noexcept
{
  Utils::ScopedErrno scoped_errno;
  const int old_flags = fcntl(fd, F_GETFL, 0);
  assert(old_flags >= 0);
  if (old_flags < 0)
    return -scoped_errno.get();

  const int new_flags = old_flags | flags;
  const int result_add_flags = fcntl(fd, F_SETFL, new_flags);
  assert(result_add_flags >= 0);
  if (result_add_flags < 0)
    return -scoped_errno.get();

  return old_flags;
}

inline int Semaphore::NonBlockingScope::set_flags(
  const int fd,
  const int flags) const noexcept
{
  Utils::ScopedErrno scoped_errno;
  const int result_set_flags = fcntl(fd, F_SETFL, flags);
  assert(result_set_flags >= 0);
  if (result_set_flags < 0)
    return -scoped_errno.get();

  return 0;
}

} // namespace UServerUtils::FileManager