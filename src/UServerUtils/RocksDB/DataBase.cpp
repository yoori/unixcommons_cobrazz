// ROCKSDB
#include <rocksdb/utilities/db_ttl.h>

// STD
#include <sstream>

// THIS
#include <UServerUtils/RocksDB/DataBase.hpp>
#include <UServerUtils/RocksDB/Logger.hpp>

namespace UServerUtils::Grpc::RocksDB
{

namespace Aspect
{

const char DATA_BASE[] = "DATA_BASE";

} // namespace Aspect

DataBase::DataBase(
  Logging::Logger* const logger,
  const std::string& db_path,
  const DBOptions& db_options,
  const Columnfamilies& column_families,
  const bool create_columns_families_if_not_exist,
  const std::optional<Ttls>& ttls)
  : logger_(ReferenceCounting::add_ref(logger))
{
  if (column_families.empty())
  {
    std::ostringstream stream;
    stream << FNS
           << "Can't create DataBase. "
              "Reason: column_families is empty";
    throw Exception(stream.str());
  }

  if (ttls && ttls->size() != column_families.size())
  {
    std::ostringstream stream;
    stream << FNS
           << "In ttl enable mode size of column_families"
              " and ttls should be the same";
    throw Exception(stream.str());
  }

  const bool is_ttl_enable = ttls.has_value();
  std::unordered_map<std::string, std::int32_t> column_family_to_ttl;
  if (is_ttl_enable)
  {
    for (std::size_t index = 0; index < column_families.size(); ++index)
    {
      column_family_to_ttl[column_families[index].name] = (*ttls)[index];
    }

    const auto it_default = column_family_to_ttl.find(
      rocksdb::kDefaultColumnFamilyName);
    if (it_default == std::end(column_family_to_ttl))
    {
      column_family_to_ttl[rocksdb::kDefaultColumnFamilyName] = 0;
    }
  }

  std::vector<ColumnFamilyDescriptor> column_families_result(
    column_families);
  std::sort(
    std::begin(column_families_result),
    std::end(column_families_result),
    [] (const auto& value1, const auto& value2) {
      return value1.name < value2.name;
    });

  for (std::size_t i = 1; i < column_families_result.size(); ++i)
  {
    if (column_families_result[i - 1].name == column_families_result[i].name)
    {
      std::ostringstream stream;
      stream << FNS
             << "Exist duplicate column family with name="
             << column_families_result[i].name;
      throw Exception(stream.str());
    }
  }

  rocksdb::ColumnFamilyOptions default_column_family_options;
  const auto it_default = std::find_if(
    std::begin(column_families_result),
    std::end(column_families_result),
    [name = rocksdb::kDefaultColumnFamilyName] (const auto& value) {
      return value.name == name;
    });
  if (it_default != std::end(column_families_result))
  {
    default_column_family_options = it_default->options;
  }
  else
  {
    const auto pos = std::upper_bound(
      std::begin(column_families_result),
      std::end(column_families_result),
      rocksdb::kDefaultColumnFamilyName,
      [] (const std::string& v, const auto& descriptor) {
        return v < descriptor.name;
      });
    column_families_result.insert(
      pos,
      ColumnFamilyDescriptor{rocksdb::kDefaultColumnFamilyName, default_column_family_options});
  }

  std::vector<std::string> existing_column_families;
  DBOptions list_columns_db_options;
  list_columns_db_options.create_if_missing = false;
  auto status = DB::ListColumnFamilies(
    list_columns_db_options,
    db_path,
    &existing_column_families);
  if (!status.ok())
  {
    if (status.subcode() == rocksdb::Status::SubCode::kPathNotFound
    && !db_options.create_if_missing)
    {
      std::ostringstream stream;
      stream << FNS
             << "Not existing rocksdb="
             << db_path;
      throw Exception(stream.str());
    }
    else if (status.subcode() != rocksdb::Status::SubCode::kPathNotFound)
    {
      std::ostringstream stream;
      stream << FNS
             << "Open DataBase is failed. Reason="
             << status.ToString();
      throw Exception(stream.str());
    }
  }
  std::sort(
    std::begin(existing_column_families),
    std::end(existing_column_families));

  std::vector<ColumnFamilyDescriptor> need_create_families;
  need_create_families.reserve(column_families.size());
  std::vector<ColumnFamilyDescriptor> already_created_families;
  already_created_families.reserve(column_families.size());
  for (const auto& descriptor : column_families_result)
  {
    const bool is_exist = std::binary_search(
      std::begin(existing_column_families),
      std::end(existing_column_families),
      descriptor.name);
    if (is_exist)
    {
      already_created_families.emplace_back(
        descriptor.name,
        descriptor.options);
    }
    else
    {
      need_create_families.emplace_back(
        descriptor.name,
        descriptor.options);
    }
  }

  if (!need_create_families.empty() && !create_columns_families_if_not_exist)
  {
    std::ostringstream stream;
    stream << FNS
           << "Not existing in db column families=";
    for (const auto& descriptor : need_create_families)
    {
      stream << descriptor.name
             << ";";
    }
    throw Exception(stream.str());
  }

  DBOptions db_options_result = db_options;
  db_options_result.info_log = std::make_shared<Logger>(logger_.in());

  if (already_created_families.empty())
  {
    rocksdb::Options options(
      db_options_result,
      default_column_family_options);
    options.create_if_missing = true;

    if (is_ttl_enable)
    {
      rocksdb::DBWithTTL* db;
      status = rocksdb::DBWithTTL::Open(
        options,
        db_path,
        &db,
        column_family_to_ttl[rocksdb::kDefaultColumnFamilyName],
        false);
      if (!status.ok())
      {
        std::ostringstream stream;
        stream << FNS
               << "Can't create empty rocsdb. Reason="
               << status.ToString();
        throw Exception(stream.str());
      }
      db_.reset(db);
    }
    else
    {
      DB* db;
      status = rocksdb::DB::Open(options, db_path, &db);
      if (!status.ok())
      {
        std::ostringstream stream;
        stream << FNS
               << "Can't create empty rocsdb. Reason="
               << status.ToString();
        throw Exception(stream.str());
      }
      db_.reset(db);
    }
  }
  else
  {
    std::vector<ColumnFamilyHandle*> handles;
    if (is_ttl_enable)
    {
      std::vector<std::int32_t> ttls_open;
      ttls_open.reserve(already_created_families.size());
      std::transform(
        std::begin(already_created_families),
        std::end(already_created_families),
        std::back_inserter(ttls_open),
        [&column_family_to_ttl] (const auto& v) {
          return column_family_to_ttl[v.name];
        });

      rocksdb::DBWithTTL *db;
      status = rocksdb::DBWithTTL::Open(
        db_options_result,
        db_path,
        already_created_families,
        &handles,
        &db,
        ttls_open,
        false);
      if (!status.ok())
      {
        std::ostringstream stream;
        stream << FNS
               << "Can't open rocksdb. Reason="
               << status.ToString();
        throw Exception(stream.str());
      }
      db_.reset(db);
    }
    else
    {
      rocksdb::DB *db;
      status = rocksdb::DB::Open(
        db_options_result,
        db_path,
        already_created_families,
        &handles,
        &db);
      if (!status.ok())
      {
        std::ostringstream stream;
        stream << FNS
               << "Can't open rocksdb. Reason="
               << status.ToString();
        throw Exception(stream.str());
      }
      db_.reset(db);
    }

    if (handles.size() != already_created_families.size())
    {
      std::ostringstream stream;
      stream << FNS
             << "Logic error. Sizes don't match";
      throw Exception(stream.str());
    }

    auto it_column_families = std::begin(already_created_families);
    for (auto it_handle_begin = std::begin(handles);
         it_handle_begin != std::end(handles);
         it_handle_begin += 1, it_column_families += 1)
    {
      auto* column_family = *it_handle_begin;
      if (!column_family)
      {
        std::ostringstream stream;
        stream << FNS
               << "ColumnFamilyHandle is null";
        throw Exception(stream.str());
      }

      column_family_handles_.try_emplace(
        it_column_families->name,
        std::make_unique<ColumnFamilyHandleWrapper>(column_family, *db_));
    }
  }

  for (const auto& descriptor : need_create_families)
  {
    if (descriptor.name == rocksdb::kDefaultColumnFamilyName)
    {
      continue;
    }

    ColumnFamilyHandle* column_family = nullptr;
    if (is_ttl_enable)
    {
      status = static_cast<rocksdb::DBWithTTL*>(db_.get())->CreateColumnFamilyWithTtl(
        descriptor.options,
        descriptor.name,
        &column_family,
        column_family_to_ttl[descriptor.name]);
    }
    else
    {
      status = db_->CreateColumnFamily(
        descriptor.options,
        descriptor.name,
        &column_family);
    }

    if (!status.ok())
    {
      std::ostringstream stream;
      stream << FNS
             << "Can't create column family="
             << descriptor.name;
      throw Exception(stream.str());
    }

    if (!column_family)
    {
      std::ostringstream stream;
      stream << FNS
             << "ColumnFamilyHandle is null";
      throw Exception(stream.str());
    }

    const auto result = column_family_handles_.try_emplace(
      descriptor.name,
      std::make_unique<ColumnFamilyHandleWrapper>(column_family, *db_));
    if (!result.second)
    {
      std::ostringstream stream;
      stream << FNS
             << "Logic error. Column family="
             << descriptor.name
             << " already exist";
      throw Exception(stream.str());
    }
  }
}

DataBase::~DataBase()
{
  try
  {
    if (!db_)
      return;

    column_family_handles_.clear();

    const auto status = db_->Close();
    if (!status.ok())
    {
      std::ostringstream stream;
      stream << FNS
             << "can't close rocksDb, reason=[code="
             << status.code()
             << ", message="
             << std::string(status.getState())
             << "]";
      logger_->error(stream.str(), Aspect::DATA_BASE);
    }
  }
  catch (...)
  {
  }
}

rocksdb::DB& DataBase::get() const noexcept
{
  return *db_.get();
}

DataBase::ColumnFamilyHandle&
DataBase::column_family(
  const std::string& name) const
{
  if (name == rocksdb::kDefaultColumnFamilyName)
  {
    return *db_->DefaultColumnFamily();
  }

  const auto it = column_family_handles_.find(name);
  if (it != std::end(column_family_handles_))
  {
    return it->second.get()->get();
  }
  else
  {
    std::ostringstream stream;
    stream << FNS
           << "Not existing column family whith name="
           << name;
    throw Exception(stream.str());
  }
}

DataBase::ColumnFamilyHandle&
DataBase::default_column_family() const
{
  return *db_->DefaultColumnFamily();
}

} // namespace UServerUtils::Grpc::RocksDB