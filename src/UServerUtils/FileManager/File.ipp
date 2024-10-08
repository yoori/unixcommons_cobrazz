// POSIX
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

// STD
#include <iostream>
#include <sstream>

// THIS
#include <Generics/Function.hpp>
#include <UServerUtils/FileManager/Utils.hpp>

namespace UServerUtils::FileManager
{

inline File::File(
  const std::string& path,
  const int flags) noexcept
{
  open(path, flags);
}

inline File::File(const File& file) noexcept
{
  if (!file.is_valid())
  {
    error_ = file.error_;
    return;
  }

  Utils::ScopedErrno scoped_errno;
  const int result = ::dup(file.fd_);
  if (result == -1)
  {
    error_ = scoped_errno.get();
    return;
  }

  fd_ = result;
  error_ = 0;
  path_ = file.path_;
}

inline File::File(File&& file) noexcept
{
  fd_ = file.fd_;
  error_ = file.error_;
  path_ = std::move(file.path_);

  file.fd_ = -1;
  file.error_ = 0;
}

inline File& File::operator=(const File& file) noexcept
{
  if (this == &file)
  {
    return *this;
  }

  close();

  if (!file.is_valid())
  {
    error_ = file.error_;
    return *this;
  }

  Utils::ScopedErrno scoped_errno;
  const int result = ::dup(file.fd_);
  if (result == -1)
  {
    error_ = scoped_errno.get();
    return *this;
  }

  fd_ = result;
  error_ = 0;
  path_ = file.path_;

  return *this;
}

inline File& File::operator=(File&& file) noexcept
{
  if (this != &file)
  {
    close();

    fd_ = file.fd_;
    error_ = file.error_;
    path_ = std::move(file.path_);

    file.fd_ = -1;
    file.error_ = 0;
  }

  return *this;
}

inline File::~File()
{
  close();
}

inline int File::fd() const noexcept
{
  return fd_;
}

inline bool File::is_valid() const noexcept
{
  return fd_ != -1;
}

inline File::operator bool() const noexcept
{
  return is_valid();
}

inline bool File::operator!() const noexcept
{
  return !is_valid();
}

inline bool File::open(
  const std::string& path,
  const int flags) noexcept
{
  close();

  Utils::ScopedErrno scoped_errno;
  const auto result = ::open(path.c_str(), flags, 0666);
  if (result == -1)
  {
    error_ = scoped_errno.get();
    return false;
  }

  fd_ = result;
  error_ = 0;
  path_ = std::make_shared<Path>(path);

  return true;
}

inline int File::error_details() const noexcept
{
  return error_;
}

inline std::string File::error_message() const
{
  if (error_ == 0)
  {
    return {};
  }
  else
  {
    return Utils::safe_strerror(error_);
  }
}

inline std::optional<std::uint64_t> File::length() const noexcept
{
  if (!is_valid())
  {
    return {};
  }

  struct stat file_info;
  if (::fstat(fd_, &file_info) == -1)
  {
    return {};
  }

  return file_info.st_size;
}

inline bool File::length(const std::int64_t length) const noexcept
{
  if (!is_valid())
  {
    return false;
  }

  if (::ftruncate(fd_, length) == -1)
  {
    return false;
  }

  return true;
}

inline timeval File::to_timeval(const Time& time) noexcept
{
  const auto millisecs =
    std::chrono::duration_cast<std::chrono::milliseconds>(
      time.time_since_epoch());

  struct timeval tv;
  tv.tv_sec  = millisecs.count() / 1000;
  tv.tv_usec = (millisecs.count() % 1000) * 1000;

  return tv;
}

inline std::chrono::system_clock::time_point File::to_time_point(
  const timeval tv) noexcept
{
  return std::chrono::system_clock::time_point{
    std::chrono::seconds{tv.tv_sec} +
      std::chrono::microseconds{tv.tv_usec}};
}

inline bool File::set_times(
  const Time& last_access_time,
  const Time& last_modified_time) noexcept
{
  if (!is_valid())
  {
    return false;
  }

  timeval times[2];
  times[0] = to_timeval(last_access_time);
  times[1] = to_timeval(last_modified_time);

  if (::futimes(fd_, times) == -1)
  {
    return false;
  }

  return true;
}

inline std::optional<File::Info> File::get_info() const noexcept
{
  if (!is_valid())
  {
    return {};
  }

  struct stat file_info;
  if (::fstat(fd_, &file_info) == -1)
  {
    return {};
  }

  Info info;
  info.size = file_info.st_size;

  const time_t last_modified_sec = file_info.st_mtim.tv_sec;
  const int64_t last_modified_nsec = file_info.st_mtim.tv_nsec;
  const time_t last_accessed_sec = file_info.st_atim.tv_sec;
  const int64_t last_accessed_nsec = file_info.st_atim.tv_nsec;
  const time_t creation_time_sec = file_info.st_ctim.tv_sec;
  const int64_t creation_time_nsec = file_info.st_ctim.tv_nsec;

  info.last_modified = Time{
    std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::seconds{last_modified_sec} +
        std::chrono::nanoseconds{last_modified_nsec})};

  info.last_accessed = Time{
    std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::seconds{last_accessed_sec} +
        std::chrono::nanoseconds{last_accessed_nsec})};

  info.creation_time = Time{
    std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::seconds{creation_time_sec} +
        std::chrono::nanoseconds{creation_time_nsec})};

  return info;
}

inline std::optional<off_t> File::lseek(
  const off_t offset,
  const int whence) const noexcept
{
  if (!is_valid())
  {
    return {};
  }

  const auto result = ::lseek(fd_, offset, whence);
  if (result == - 1)
  {
    return {};
  }

  return result;
}

inline void File::close() noexcept
{
  if (is_valid())
  {
    ::close(fd_);
  }

  fd_ = -1;
  error_ = 0;
  path_.reset();
}

inline const std::string& File::path() const noexcept
{
  static const std::string empty_path;
  return path_ ? *path_ : empty_path;
}

} // namespace UServerUtils::FileManager