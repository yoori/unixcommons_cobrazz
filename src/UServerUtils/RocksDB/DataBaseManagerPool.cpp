// THIS
#include <UServerUtils/RocksDB/DataBaseManager.hpp>
#include <UServerUtils/RocksDB/DataBaseManagerPool.hpp>

namespace UServerUtils::Grpc::RocksDB
{

DataBaseManagerPool::DataBaseManagerPool(
  const Config& config,
  Logging::Logger* logger)
{
  db_managers_.reserve(config.number_io_urings);
  if ((config.io_uring_flags & IORING_SETUP_ATTACH_WQ) == 0)
  {
    for (std::size_t i = 1; i <= config.number_io_urings; ++i)
    {
      db_managers_.emplace_back(
        std::make_unique<DataBaseManager>(config, logger));
    }
  }
  else
  {
    Config helper_config(config);
    helper_config.io_uring_flags &= ~IORING_SETUP_ATTACH_WQ;
    auto db_manager = std::make_unique<DataBaseManager>(helper_config, logger);
    const auto uring_fd = db_manager->uring_fd();
    db_managers_.emplace_back(std::move(db_manager));

    for (std::size_t i = 1; i < config.number_io_urings; ++i)
    {
      db_managers_.emplace_back(
        std::make_unique<DataBaseManager>(config, uring_fd, logger));
    }
  }
}

DataBaseManagerPool::DataBaseManagerPool(
  const Config& config,
  const std::uint32_t uring_fd,
  Logging::Logger* logger)
{
  for (std::size_t i = 1; i <= config.number_io_urings; ++i)
  {
    db_managers_.emplace_back(
      std::make_unique<DataBaseManager>(config, uring_fd, logger));
  }
}

std::size_t DataBaseManagerPool::size() const noexcept
{
  return db_managers_.size();
}

void DataBaseManagerPool::get(
  const DataBasePtr& db,
  ColumnFamilyHandle& column_family,
  const ReadOptions& read_options,
  const std::string_view key,
  GetCallback&& callback) noexcept
{
  const auto index =
    counter_.fetch_add(1, std::memory_order_relaxed) % db_managers_.size();
  auto& db_manager = db_managers_[index];
  db_manager->get(
    db,
    column_family,
    read_options,
    key,
    std::move(callback));
}

DataBaseManagerPool::Status DataBaseManagerPool::get(
  const DataBasePtr& db,
  ColumnFamilyHandle& column_family,
  const ReadOptions& read_options,
  const std::string_view key,
  std::string& value) noexcept
{
  const auto index =
    counter_.fetch_add(1, std::memory_order_relaxed) % db_managers_.size();
  auto& db_manager = db_managers_[index];
  return db_manager->get(
    db,
    column_family,
    read_options,
    key,
    value);
}

void DataBaseManagerPool::multi_get(
  const DataBasePtr& db,
  ColumnFamilies&& column_families,
  const ReadOptions& read_options,
  Keys&& keys,
  MultiGetCallback&& callback) noexcept
{
  const auto index =
    counter_.fetch_add(1, std::memory_order_relaxed) % db_managers_.size();
  auto& db_manager = db_managers_[index];
  db_manager->multi_get(
    db,
    std::move(column_families),
    read_options,
    std::move(keys),
    std::move(callback));
}

DataBaseManagerPool::Statuses DataBaseManagerPool::multi_get(
  const DataBasePtr& db,
  ColumnFamilies&& column_families,
  const ReadOptions& read_options,
  Keys&& keys,
  Values& values) noexcept
{
  const auto index =
    counter_.fetch_add(1, std::memory_order_relaxed) % db_managers_.size();
  auto& db_manager = db_managers_[index];
  return db_manager->multi_get(
    db,
    std::move(column_families),
    read_options,
    std::move(keys),
    values);
}

void DataBaseManagerPool::put(
  const DataBasePtr& db,
  ColumnFamilyHandle& column_family,
  const WriteOptions& write_options,
  const std::string_view key,
  const std::string_view value,
  PutCallback&& callback) noexcept
{
  const auto index =
    counter_.fetch_add(1, std::memory_order_relaxed) % db_managers_.size();
  auto& db_manager = db_managers_[index];
  db_manager->put(
    db,
    column_family,
    write_options,
    key,
    value,
    std::move(callback));
}

DataBaseManagerPool::Status DataBaseManagerPool::put(
  const DataBasePtr& db,
  ColumnFamilyHandle& column_family,
  const WriteOptions& write_options,
  const std::string_view key,
  const std::string_view value) noexcept
{
  const auto index =
    counter_.fetch_add(1, std::memory_order_relaxed) % db_managers_.size();
  auto& db_manager = db_managers_[index];
  return db_manager->put(
    db,
    column_family,
    write_options,
    key,
    value);
}

void DataBaseManagerPool::erase(
  const DataBasePtr& db,
  ColumnFamilyHandle& column_family,
  const WriteOptions& write_options,
  const std::string_view key,
  EraseCallback&& callback) noexcept
{
  const auto index =
    counter_.fetch_add(1, std::memory_order_relaxed) % db_managers_.size();
  auto& db_manager = db_managers_[index];
  db_manager->erase(
    db,
    column_family,
    write_options,
    key,
    std::move(callback));
}

DataBaseManagerPool::Status DataBaseManagerPool::erase(
  const DataBasePtr& db,
  ColumnFamilyHandle& column_family,
  const WriteOptions& write_options,
  const std::string_view key) noexcept
{
  const auto index =
    counter_.fetch_add(1, std::memory_order_relaxed) % db_managers_.size();
  auto& db_manager = db_managers_[index];
  return db_manager->erase(
    db,
    column_family,
    write_options,
    key);
}

} // namespace UServerUtils::Grpc::RocksDB