// THIS
#include <UServerUtils/Grpc/FileManager/FileManagerPool.hpp>

namespace UServerUtils::Grpc::FileManager
{

FileManagerPool::FileManagerPool(
  const Config& config,
  Logging::Logger* logger)
{
  file_managers_.reserve(config.number_io_urings);
  for (std::size_t i = 1; i <= config.number_io_urings; ++i)
  {
    file_managers_.emplace_back(
      std::make_unique<FileManager>(config, logger));
  }
}

std::size_t FileManagerPool::size() const noexcept
{
  return file_managers_.size();
}

void FileManagerPool::write(
  const File& file,
  const std::string_view buffer,
  const std::int64_t offset,
  Callback&& callback) noexcept
{
  const std::size_t index =
      counter_.fetch_add(1, std::memory_order_relaxed) % file_managers_.size();
  auto& file_manager = file_managers_[index];
  file_manager->write(
    file,
    buffer,
    offset,
    std::move(callback));
}

int FileManagerPool::FileManagerPool::write(
  const File& file,
  const std::string_view buffer,
  const std::int64_t offset) noexcept
{
  const std::size_t index =
    counter_.fetch_add(1, std::memory_order_relaxed) % file_managers_.size();
  auto& file_manager = file_managers_[index];
  return file_manager->write(
    file,
    buffer,
    offset);
}

void FileManagerPool::read(
  const File& file,
  const std::string_view buffer,
  const std::int64_t offset,
  Callback&& callback) noexcept
{
  const std::size_t index =
    counter_.fetch_add(1, std::memory_order_relaxed) % file_managers_.size();
  auto& file_manager = file_managers_[index];
  file_manager->read(
    file,
    buffer,
    offset,
    std::move(callback));
}

int FileManagerPool::read(
  const File& file,
  const std::string_view buffer,
  const std::int64_t offset) noexcept
{
  const std::size_t index =
    counter_.fetch_add(1, std::memory_order_relaxed) % file_managers_.size();
  auto& file_manager = file_managers_[index];
  return file_manager->read(
    file,
    buffer,
    offset);
}

} // namespace UServerUtils::Grpc::FileManager
