#ifndef USERVER_FILEMANAGER_UTILS_HPP
#define USERVER_FILEMANAGER_UTILS_HPP

// POSIX
#include <errno.h>
#include <stdlib.h>
#include <string.h>

// STD
#include <bit>
#include <cassert>
#include <sstream>
#include <string>

// THIS
#include <eh/Exception.hpp>
#include <Generics/Function.hpp>
#include <Generics/Uncopyable.hpp>

#define assertm(exp, msg) assert(((void)msg, exp))

namespace UServerUtils::FileManager::Utils
{

namespace Internal
{

#if defined(__GLIBC__)
#define USE_HISTORICAL_STRERROR_R 1
#else
#define USE_HISTORICAL_STRERROR_R 0
#endif

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

template <class T, class = std::enable_if_t<std::is_integral_v<T>>>
constexpr bool is_power_of_two(T t) noexcept
{
  return std::has_single_bit(t);
}

#ifdef __has_builtin
#define SUPPORTS_BUILTIN_IS_ALIGNED (__has_builtin(__builtin_is_aligned))
#else
#define SUPPORTS_BUILTIN_IS_ALIGNED 0
#endif

inline bool is_aligned(const void* val, const std::size_t alignment) noexcept
{
#if SUPPORTS_BUILTIN_IS_ALIGNED
  return __builtin_is_aligned(val, alignment);
#else
  assertm(is_power_of_two(alignment), "alignment is not a power of 2");
  return (reinterpret_cast<uintptr_t>(val) & (alignment - 1)) == 0;
#endif
}

} // namespace Internal

inline std::string safe_strerror(const int error)
{
  const int buffer_size = 256;
  char buffer[buffer_size];
  Internal::safe_strerror_r(error, buffer, sizeof(buffer));
  return std::string(buffer);
}

inline void* aligned_alloc(const std::size_t size, const std::size_t alignment)
{
  assert(size != 0);
  assert(Internal::is_power_of_two(alignment));
  assert(alignment % sizeof(void*) == 0);

  void* ptr = nullptr;
  const int ret = posix_memalign(&ptr, alignment, size);
  if (ret != 0)
  {
    std::ostringstream stream;
    stream << FNS
           << safe_strerror(ret);
    throw std::runtime_error(stream.str().c_str());
  }

  assert(Internal::is_aligned(ptr, alignment));
  return ptr;
}

inline void aligned_free(void* ptr) noexcept
{
  free(ptr);
}

struct AlignedFreeDeleter {
  void operator()(void* ptr) const noexcept
  {
    aligned_free(ptr);
  }
};

class ScopedErrno final : private Generics::Uncopyable
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

} // namespace UServerUtils::FileManager::Utils

#endif // USERVER_FILEMANAGER_UTILS_HPP
