#ifndef USERVER_ROCKSDB_DATABASEMANAGERPOOL_HPP
#define USERVER_ROCKSDB_DATABASEMANAGERPOOL_HPP

// STD
#include <atomic>
#include <memory>
#include <vector>

// THIS
#include <UServerUtils/Grpc/RocksDB/DataBaseManager.hpp>

namespace UServerUtils::Grpc::RocksDB
{

class DataBaseManagerPool final : private Generics::Uncopyable
{
public:
  using Logger = DataBaseManager::Logger;
  using Logger_var = DataBaseManager::Logger_var;
  using ColumnFamilyHandle = DataBaseManager::ColumnFamilyHandle;
  using ColumnFamilies = DataBaseManager::ColumnFamilies;
  using DB = DataBaseManager::DB;
  using Status = DataBaseManager::Status;
  using Statuses = DataBaseManager::Statuses;
  using ReadOptions = DataBaseManager::ReadOptions;
  using GetCallback = DataBaseManager::GetCallback;
  using Keys = DataBaseManager::Keys;
  using Values = DataBaseManager::Values;
  using MultiGetCallback = DataBaseManager::MultiGetCallback;
  using WriteOptions = DataBaseManager::WriteOptions;
  using PutCallback = DataBaseManager::PutCallback;
  using EraseCallback = DataBaseManager::EraseCallback;
  using DataBasePtr = DataBaseManager::DataBasePtr;

private:
  using DataBaseManagerPtr = std::unique_ptr<DataBaseManager>;
  using DataBaseManagers = std::vector<DataBaseManagerPtr>;
  using Counter = std::atomic<std::uint64_t>;

public:
  explicit DataBaseManagerPool(
    const Config& config,
    Logging::Logger* logger);

  explicit DataBaseManagerPool(
    const Config& config,
    const std::uint32_t uring_fd,
    Logging::Logger* logger);

  ~DataBaseManagerPool() = default;

  std::size_t size() const noexcept;

  void get(
    const DataBasePtr& db,
    ColumnFamilyHandle& column_family,
    const ReadOptions& read_options,
    const std::string_view key,
    GetCallback&& callback) noexcept;

  Status get(
    const DataBasePtr& db,
    ColumnFamilyHandle& column_family,
    const ReadOptions& read_options,
    const std::string_view key,
    std::string& value) noexcept;

  void multi_get(
    const DataBasePtr& db,
    ColumnFamilies&& column_families,
    const ReadOptions& read_options,
    Keys&& keys,
    MultiGetCallback&& callback) noexcept;

  Statuses multi_get(
    const DataBasePtr& db,
    ColumnFamilies&& column_families,
    const ReadOptions& read_options,
    Keys&& keys,
    Values& values) noexcept;

  void put(
    const DataBasePtr& db,
    ColumnFamilyHandle& column_family,
    const WriteOptions& write_options,
    const std::string_view key,
    const std::string_view value,
    PutCallback&& callback) noexcept;

  Status put(
    const DataBasePtr& db,
    ColumnFamilyHandle& column_family,
    const WriteOptions& write_options,
    const std::string_view key,
    const std::string_view value) noexcept;

  void erase(
    const DataBasePtr& db,
    ColumnFamilyHandle& column_family,
    const WriteOptions& write_options,
    const std::string_view key,
    EraseCallback&& callback) noexcept;

  Status erase(
    const DataBasePtr& db,
    ColumnFamilyHandle& column_family,
    const WriteOptions& write_options,
    const std::string_view key) noexcept;

private:
  Counter counter_{0};

  DataBaseManagers db_managers_;
};

} // namespace UServerUtils::Grpc::RocksDB

#endif // USERVER_ROCKSDB_DATABASEMANAGERPOOL_HPP