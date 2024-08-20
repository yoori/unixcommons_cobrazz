#ifndef USERVER_FILEMANAGER_FILEMANAGERPOOL_HPP
#define USERVER_FILEMANAGER_FILEMANAGERPOOL_HPP

// STD
#include <atomic>
#include <memory>
#include <vector>

// THIS
#include <UServerUtils/FileManager/FileManager.hpp>

namespace UServerUtils::FileManager
{

class FileManagerPool final : private Generics::Uncopyable
{
private:
  using FileManagerPtr = std::unique_ptr<FileManager>;
  using FileManagers = std::vector<FileManagerPtr>;
  using Counter = std::atomic<std::uint64_t>;

public:
  using Callback = typename FileManager::Callback;

public:
  explicit FileManagerPool(
    const Config& config,
    Logging::Logger* logger);

  explicit FileManagerPool(
    const Config& config,
    const std::uint32_t uring_fd,
    Logging::Logger* logger);

  ~FileManagerPool() = default;

  std::size_t size() const noexcept;

  /**
 * You must ensure that buffer survives callback.
 * On success, the number of written bytes pass to callback.
 * On error, -error pass to callback.
 * */
  void write(
    const File& file,
    const std::string_view buffer,
    const std::int64_t offset,
    Callback&& callback) noexcept;

  /*
   * If call from coroutine, block coroutine.
   * Otherwise block thread.
   * On success, return number of written bytes.
   * On error, return -error.
   * */
  int write(
    const File& file,
    const std::string_view buffer,
    const std::int64_t offset) noexcept;

  /**
   * You must ensure that buffer survives callback.
   * On success, the number of read bytes pass to callback.
   * On error, -error pass to callback.
   * */
  void read(
    const File& file,
    const std::string_view buffer,
    const std::int64_t offset,
    Callback&& callback) noexcept;

  /**
   * If call from coroutine, block coroutine.
   * Otherwise block thread.
   * On success, return number of read bytes.
   * On error, return -error.
   **/
  int read(
    const File& file,
    const std::string_view buffer,
    const std::int64_t offset) noexcept;

private:
  Counter counter_{0};

  FileManagers file_managers_;
};

using FileManagerPoolPtr = std::shared_ptr<FileManagerPool>;

} // namespace UServerUtils::FileManager

#endif // USERVER_FILEMANAGER_FILEMANAGERPOOL_HPP