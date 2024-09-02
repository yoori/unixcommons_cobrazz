#ifndef USERVER_ROCKSDB_DATABASE_HPP
#define USERVER_ROCKSDB_DATABASE_HPP

// RocksDB
#include "rocksdb/db.h"
#include "rocksdb/options.h"

// STD
#include <memory>
#include <optional>

// THIS
#include <eh/Exception.hpp>
#include <Logger/Logger.hpp>

namespace UServerUtils::Grpc::RocksDB
{

class DataBase final : private Generics::Uncopyable
{
public:
  using Logger_var = Logging::Logger_var;
  using DBOptions = rocksdb::DBOptions;
  using ColumnFamilyOptions = rocksdb::ColumnFamilyOptions;
  using DB = rocksdb::DB;
  using DBPtr = std::unique_ptr<rocksdb::DB>;
  using Code = rocksdb::Status::Code;
  using ColumnFamilyDescriptor = rocksdb::ColumnFamilyDescriptor;
  using Columnfamilies = std::vector<ColumnFamilyDescriptor>;
  using ColumnFamilyHandle = rocksdb::ColumnFamilyHandle;
  using Ttls = std::vector<std::int32_t>;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

private:
  class ColumnFamilyHandleWrapper;
  using ColumnFamilyHandleWrapperPtr =
    std::unique_ptr<ColumnFamilyHandleWrapper>;
  using ColumnFamilyHandles =
    std::unordered_map<std::string, ColumnFamilyHandleWrapperPtr>;

public:
  explicit DataBase(
    Logging::Logger* const logger,
    const std::string& db_path,
    const DBOptions& db_options,
    const Columnfamilies& column_families,
    const bool create_columns_families_if_not_exist = true,
    const std::optional<Ttls>& ttls = {});

  ~DataBase();

  DB& get() const noexcept;

  // If column family does not exist throw Exception.
  ColumnFamilyHandle& column_family(const std::string& name) const;

  ColumnFamilyHandle& default_column_family() const;

private:
  const Logger_var logger_;

  ColumnFamilyHandles column_family_handles_;

  DBPtr db_;
};

class DataBase::ColumnFamilyHandleWrapper
  : private Generics::Uncopyable
{
public:
  ColumnFamilyHandleWrapper(
    ColumnFamilyHandle* handle,
    DB& db)
    : handle_(handle),
      db_(db)
  {
  }

  ColumnFamilyHandle& get() noexcept
  {
    return *handle_;
  }

  ~ColumnFamilyHandleWrapper()
  {
    try
    {
      db_.DestroyColumnFamilyHandle(handle_);
    }
    catch (...)
    {
    }
  }

private:
  ColumnFamilyHandle* handle_;

  DB& db_;
};

using DataBasePtr = std::shared_ptr<DataBase>;

} // namespace UServerUtils::Grpc::RocksDB

#include <UServerUtils/RocksDB/DataBase.ipp>

#endif // USERVER_ROCKSDB_DATABASE_HPP