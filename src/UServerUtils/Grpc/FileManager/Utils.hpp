#ifndef USERVER_FILEMANAGER_UTILS_HPP
#define USERVER_FILEMANAGER_UTILS_HPP

// POSIX
#include <errno.h>
#include <string.h>

// STD
#include <string>

namespace UServerUtils::Grpc::FileManager::Utils
{

#if defined(__GLIBC__)
#define USE_HISTORICAL_STRERROR_R 1
#elif defined(__BIONIC__) && defined(_GNU_SOURCE) && __ANDROID_API__ >= 23
#define USE_HISTORICAL_STRERROR_R 1
#else
#define USE_HISTORICAL_STRERROR_R 0
#endif

namespace Internal
{

#if USE_HISTORICAL_STRERROR_R
inline void wrap_posix_strerror_r(
  char* (*strerror_r_ptr)(int, char*, size_t),
  int error,
  char* buffer,
  size_t length)
{
  char* result = (*strerror_r_ptr)(error, buffer, length);
  if (result != buffer)
  {
    buffer[0] = '\0';
    strncat(buffer, result, length - 1);
  }
}
#else
inline void wrap_posix_strerror_r(
  int (*strerror_r_ptr)(int, char*, size_t),
  const int error,
  char* buffer,
  const size_t length)
{
  const int old_errno = errno;
  const int result = (*strerror_r_ptr)(error, buffer, length);
  if (result == 0)
  {
    buffer[length - 1] = '\0';
  }
  else
  {
    int strerror_error;
    int new_errno = errno;
    if (new_errno != old_errno)
    {
      strerror_error = new_errno;
    } else
    {
      strerror_error = result;
    }

    snprintf(
      buffer,
      length,
      "Error %d while retrieving error %d",
      strerror_error,
      error);
  }
  errno = old_errno;
}
#endif  // USE_HISTORICAL_STRERROR_R

inline void safe_strerror_r(int error, char* buffer, size_t length)
{
  if (buffer == nullptr || length <= 0)
    return;

  wrap_posix_strerror_r(&strerror_r, error, buffer, length);
}

} // namespace Internal

inline std::string safe_strerror(const int error)
{
  const int buffer_size = 256;
  char buffer[buffer_size];
  Internal::safe_strerror_r(error, buffer, sizeof(buffer));
  return std::string(buffer);
}

class ScopedErrno final
{
public:
  ScopedErrno() noexcept
  {
    old_errno_ = errno;
    errno = 0;
  }

  ~ScopedErrno() noexcept
  {
    errno = old_errno_;
  }

  int get() const noexcept
  {
    return errno;
  }

private:
  int old_errno_ = 0;
};

}; // namespace UServerUtils::Grpc::FileManager::Utils

#endif // USERVER_FILEMANAGER_UTILS_HPP