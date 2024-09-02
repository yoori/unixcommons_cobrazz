// THIS
#include <UServerUtils/FileManager/FileManagerPool.hpp>

namespace UServerUtils::FileManager
{

inline FileManagerPool::FileManagerPool(
  const Config& config,
  Logging::Logger* logger)
{
  file_managers_.reserve(config.number_io_urings);
  if ((config.io_uring_flags & IORING_SETUP_ATTACH_WQ) == 0)
  {
    for (std::size_t i = 1; i <= config.number_io_urings; ++i)
    {
      file_managers_.emplace_back(
        std::make_unique<FileManager>(config, logger));
    }
  }
  else
  {
    Config helper_config(config);
    helper_config.io_uring_flags &= ~IORING_SETUP_ATTACH_WQ;
    auto file_manager = std::make_unique<FileManager>(helper_config, logger);
    const auto uring_fd = file_manager->uring_fd();
    file_managers_.emplace_back(std::move(file_manager));

    for (std::size_t i = 1; i < config.number_io_urings; ++i)
    {
      file_managers_.emplace_back(
        std::make_unique<FileManager>(config, uring_fd, logger));
    }
  }
}

inline FileManagerPool::FileManagerPool(
  const Config& config,
  const std::uint32_t uring_fd,
  Logging::Logger* logger)
{
  for (std::size_t i = 1; i <= config.number_io_urings; ++i)
  {
    file_managers_.emplace_back(
      std::make_unique<FileManager>(config, uring_fd, logger));
  }
}

inline std::size_t FileManagerPool::size() const noexcept
{
  return file_managers_.size();
}

inline void FileManagerPool::write(
  const File& file,
  const std::string_view buffer,
  const std::int64_t offset,
  Callback&& callback) noexcept
{
  const auto index = counter_.fetch_add(
    1,
    std::memory_order_relaxed) % file_managers_.size();
  auto& file_manager = file_managers_[index];
  file_manager->write(
    file,
    buffer,
    offset,
    std::move(callback));
}

inline int FileManagerPool::FileManagerPool::write(
  const File& file,
  const std::string_view buffer,
  const std::int64_t offset) noexcept
{
  const auto index = counter_.fetch_add(
    1,
    std::memory_order_relaxed) % file_managers_.size();
  auto& file_manager = file_managers_[index];
  return file_manager->write(
    file,
    buffer,
    offset);
}

inline void FileManagerPool::read(
  const File& file,
  const std::string_view buffer,
  const std::int64_t offset,
  Callback&& callback) noexcept
{
  const auto index = counter_.fetch_add(
    1,
    std::memory_order_relaxed) % file_managers_.size();
  auto& file_manager = file_managers_[index];
  file_manager->read(
    file,
    buffer,
    offset,
    std::move(callback));
}

inline int FileManagerPool::read(
  const File& file,
  const std::string_view buffer,
  const std::int64_t offset) noexcept
{
  const auto index = counter_.fetch_add(
    1,
    std::memory_order_relaxed) % file_managers_.size();
  auto& file_manager = file_managers_[index];
  return file_manager->read(
    file,
    buffer,
    offset);
}

} // namespace UServerUtils::FileManager
