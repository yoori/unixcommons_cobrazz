#ifndef USERVER_FILEMANAGER_FILE_HPP
#define USERVER_FILEMANAGER_FILE_HPP

// POSIX
#include <fcntl.h>

// STD
#include <chrono>
#include <optional>
#include <string>

// THIS
#include <eh/Exception.hpp>

namespace UServerUtils::Grpc::FileManager
{

class File final
{
public:
  using Time = std::chrono::time_point<std::chrono::system_clock>;

  struct Info final
  {
    Info() = default;
    ~Info() = default;

    std::uint64_t size = 0;
    Time last_modified = {};
    Time last_accessed = {};
    Time creation_time = {};
  };

public:
  File() noexcept = default;

  explicit File(
    const std::string& path,
    const int flags = O_CREAT | O_RDWR | O_APPEND) noexcept;

  File(const File& file) noexcept;

  File(File&& file) noexcept;

  File& operator=(const File& file) noexcept;

  File& operator=(File&& file) noexcept;

  ~File() noexcept;

  bool is_valid() const noexcept;

  bool open(
    const std::string& path,
    const int flags = O_CREAT | O_APPEND) noexcept;

  int error_details() const noexcept;

  std::string error_message() const;

  int fd() const noexcept;

  std::optional<off_t> lseek(
    const off_t offset,
    const int whence) const noexcept;

  void close() noexcept;

  std::optional<std::uint64_t> get_length() const noexcept;

  bool set_length(const std::int64_t length) const noexcept;

  bool set_times(
    const Time& last_access_time,
    const Time& last_modified_time) noexcept;

  std::optional<Info> get_info() const noexcept;

private:
  timeval to_timeval(
    const Time& time) noexcept;

  std::chrono::system_clock::time_point to_time_point(
    const timeval tv) noexcept;

private:
  int fd_ = -1;

  int error_ = 0;
};

} // namespace UServerUtils::Grpc::FileManager

#endif // USERVER_FILEMANAGER_FILE_HPP