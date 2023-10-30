// GTEST
#include <gtest/gtest.h>

// STD
#include <cstdio>
#include <filesystem>
#include <future>
#include <memory>
#include <string>

// THIS
#include <Logger/Logger.hpp>
#include <Logger/StreamLogger.hpp>
#include <UServerUtils/Grpc/RocksDB/DataBase.hpp>
#include <UServerUtils/Grpc/RocksDB/DataBaseManager.hpp>
#include <UServerUtils/Grpc/RocksDB/DataBaseManagerPool.hpp>

using namespace UServerUtils::Grpc::RocksDB;

namespace
{

[[maybe_unused]] rocksdb::DBOptions create_db_options()
{
  rocksdb::DBOptions options;
  options.IncreaseParallelism(5);
  options.create_if_missing = true;
  return options;
}

[[maybe_unused]] rocksdb::ColumnFamilyOptions create_column_family_options()
{
  rocksdb::ColumnFamilyOptions options;
  options.OptimizeForPointLookup(10);
  return options;
}

[[maybe_unused]] void remove_directory(const std::string& str_path)
{
  std::filesystem::path path(str_path);
  std::filesystem::remove_all(path);
}

[[maybe_unused]] std::unique_ptr<DataBase> create_rocksdb(
  const std::string& column_family_name,
  Logging::Logger* logger,
  const bool need_recreate = true)
{
  const std::string path_db = "/tmp/rocks_db";
  if (need_recreate)
  {
    remove_directory(path_db);
  }

  auto db_options = create_db_options();
  db_options.create_if_missing = true;
  auto column_family_options = create_column_family_options();

  std::vector<rocksdb::ColumnFamilyDescriptor> descriptors{
    {column_family_name, column_family_options}};
  return std::make_unique<DataBase>(
    logger,
    path_db,
    db_options,
    descriptors,
    true);
}

} // namespace

TEST(ROCKSDB, DataBase)
{
  Logging::Logger_var logger(
    new Logging::OStream::Logger(
      Logging::OStream::Config(
        std::cerr,
        Logging::Logger::ERROR)));

  const std::string path_db = "/tmp/rocks_db";

  {
    remove_directory(path_db);
    auto db_options = create_db_options();
    db_options.create_if_missing = true;
    auto column_family_options = create_column_family_options();

    EXPECT_THROW(DataBase database(
      logger.in(),
      path_db,
      db_options,
      {{"col1", column_family_options}},
      false), eh::Exception);
  }

  {
    remove_directory(path_db);
    auto db_options = create_db_options();
    db_options.create_if_missing = false;
    auto column_family_options = create_column_family_options();

    EXPECT_THROW(DataBase database(
      logger.in(),
      path_db,
      db_options,
      {{"col1", column_family_options}},
      true), eh::Exception);
  }

  {
    remove_directory(path_db);

    {
      auto db_options = create_db_options();
      db_options.create_if_missing = true;
      auto column_family_options = create_column_family_options();

      DataBase database(
        logger.in(),
        path_db,
        db_options,
        {{"col1", column_family_options}},
        true);

      EXPECT_TRUE(database.get().GetDBOptions().create_if_missing);
    }

    {
      auto db_options = create_db_options();
      db_options.create_if_missing = false;
      auto column_family_options = create_column_family_options();

      DataBase database(
        logger.in(),
        path_db,
        db_options,
        {{"col1", column_family_options}},
        false);

      EXPECT_FALSE(database.get().GetDBOptions().create_if_missing);
    }

    {
      auto db_options = create_db_options();
      db_options.create_if_missing = false;
      auto column_family_options = create_column_family_options();

      EXPECT_THROW(DataBase database(
        logger.in(),
        path_db,
        db_options,
        {{"col1", column_family_options}, {"col2", column_family_options}},
        false), eh::Exception);
    }

    {
      auto db_options = create_db_options();
      db_options.create_if_missing = false;
      auto column_family_options = create_column_family_options();

      DataBase database(
        logger.in(),
        path_db,
        db_options,
        {{"col1", column_family_options}, {"col2", column_family_options}},
        true);
      EXPECT_FALSE(database.get().GetDBOptions().create_if_missing);
    }

    {
      auto db_options = create_db_options();
      db_options.create_if_missing = false;
      auto column_family_options = create_column_family_options();

      DataBase database(
        logger.in(),
        path_db,
        db_options,
        {{"col1", column_family_options}, {"col2", column_family_options}},
        false);
      EXPECT_FALSE(database.get().GetDBOptions().create_if_missing);
    }

    {
      auto db_options = create_db_options();
      db_options.create_if_missing = false;
      auto column_family_options = create_column_family_options();

      EXPECT_THROW(DataBase database(
        logger.in(),
        path_db,
        db_options,
        {{"col1", column_family_options}},
        false), eh::Exception);
    }
  }
}

TEST(DataBaseManagerTest, DataBaseManagerDestroy)
{
  Logging::Logger_var logger(
    new Logging::OStream::Logger(
      Logging::OStream::Config(
        std::cerr,
        Logging::Logger::ERROR)));
  for (int i = 0; i <= 100; ++i)
  {
    Config config;
    config.io_uring_flags = 0;
    config.io_uring_size = 10;
    DataBaseManager data_base_manager(config, logger.in());
  }

  EXPECT_TRUE(true);
}

TEST(DataBaseManagerTest, Get)
{
  Logging::Logger_var logger(
    new Logging::OStream::Logger(
      Logging::OStream::Config(
        std::cerr,
        Logging::Logger::ERROR)));

  const std::string column_family_name = "column";
  auto data_base = create_rocksdb(column_family_name, logger.in());
  auto& column_family_handle = data_base->column_family(column_family_name);

  Config config;
  config.io_uring_flags = 0;
  config.io_uring_size = 10;
  DataBaseManager data_base_manager(config, logger.in());

  std::string key = "key";
  std::string value = "value";

  {
    std::promise<void> promise;
    auto future = promise.get_future();

    data_base_manager.get(
    *data_base,
    column_family_handle,
    rocksdb::ReadOptions{},
    key,
    [promise = std::move(promise)] (
      const rocksdb::Status& status,
      const std::string_view value) mutable {
        EXPECT_EQ(status.code(), rocksdb::Status::Code::kNotFound);
        promise.set_value();
    });

    future.wait();
  }

  rocksdb::WriteOptions write_options;
  write_options.sync = true;
  auto status = data_base->get().Put(
    write_options,
    &column_family_handle,
    key,
    value);
  EXPECT_TRUE(status.ok());

  {
    std::promise<void> promise;
    auto future = promise.get_future();

    data_base_manager.get(
      *data_base,
      column_family_handle,
      rocksdb::ReadOptions{},
      key,
      [promise = std::move(promise), value] (
        const rocksdb::Status& status,
        const std::string_view result) mutable {
        EXPECT_TRUE(status.ok());
        EXPECT_EQ(value, result);
        promise.set_value();
      });

    future.wait();
  }

  {
    std::string result;
    status = data_base_manager.get(
      *data_base,
      column_family_handle,
      rocksdb::ReadOptions{},
      key,
      result);
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(result, value);
  }

  {
    const std::size_t number = 3000;
    std::atomic<std::size_t> count{0};
    {
      DataBaseManager data_base_manager2(config, logger.in());
      for (std::size_t i = 1; i <= number; ++i)
      {
        data_base_manager2.get(
          *data_base,
          column_family_handle,
          rocksdb::ReadOptions{},
          key,
          [&count, value] (
            const rocksdb::Status& status,
            const std::string_view result) mutable {
            EXPECT_TRUE(status.ok());
            EXPECT_EQ(value, result);
            count.fetch_add(1, std::memory_order_relaxed);
          });
      }
    }
    EXPECT_EQ(number, count.load());
  }
}

TEST(DataBaseManagerTest, MultiGet) {
  Logging::Logger_var logger(
    new Logging::OStream::Logger(
      Logging::OStream::Config(
        std::cerr,
        Logging::Logger::ERROR)));

  const std::string column_family_name = "column";
  auto data_base = create_rocksdb(column_family_name, logger.in());
  auto &column_family_handle = data_base->column_family(column_family_name);

  Config config;
  config.io_uring_flags = 0;
  config.io_uring_size = 10;
  DataBaseManager data_base_manager(config, logger.in());

  std::string key1 = "key1";
  std::string value1 = "value1";

  std::string key2 = "key2";
  std::string value2 = "value2";

  std::string key3 = "key3";
  std::string value3 = "value2";

  {
    rocksdb::WriteOptions write_options;
    write_options.sync = true;

    auto status = data_base->get().Put(
      write_options,
      &column_family_handle,
      key1,
      value1);
    EXPECT_TRUE(status.ok());

    status = data_base->get().Put(
      write_options,
      &column_family_handle,
      key2,
      value2);
    EXPECT_TRUE(status.ok());

    status = data_base->get().Put(
      write_options,
      &column_family_handle,
      key3,
      value3);
    EXPECT_TRUE(status.ok());
  }

  {
    std::promise<void> promise;
    auto future = promise.get_future();
    data_base_manager.multi_get(
      *data_base,
      {&column_family_handle, &column_family_handle},
      rocksdb::ReadOptions{},
      {key1, key2},
      [promise = std::move(promise), &value1, &value2] (
        const DataBaseManager::Status& status,
        DataBaseManager::Values&& values) {
          EXPECT_TRUE(status.ok());
          EXPECT_EQ(values.size(), 2);
          if (values.size() == 2)
          {
            EXPECT_EQ(values[0], value1);
            EXPECT_EQ(values[1], value2);
          }
      });
    future.wait();
  }

  {
    std::promise<void> promise;
    auto future = promise.get_future();
    data_base_manager.multi_get(
      *data_base,
      {&column_family_handle, &column_family_handle},
      rocksdb::ReadOptions{},
      {key1, std::string("not_exist")},
      [promise = std::move(promise), &value1, &value2] (
        const DataBaseManager::Status& status,
        DataBaseManager::Values&& values) {
        EXPECT_TRUE(status.ok());
        EXPECT_EQ(values.size(), 2);
        if (values.size() == 2)
        {
          EXPECT_EQ(values[0], value1);
          EXPECT_TRUE(values[1].empty());
        }
      });
    future.wait();
  }

  {
    std::vector<std::string> values;
    const auto status = data_base_manager.multi_get(
      *data_base,
      {&column_family_handle, &column_family_handle},
      rocksdb::ReadOptions{},
      {key1, key2},
      values);

    EXPECT_TRUE(status.ok());
    EXPECT_EQ(values.size(), 2);
    if (values.size() == 2)
    {
      EXPECT_EQ(values[0], value1);
      EXPECT_EQ(values[1], value2);
    }
  }
}

TEST(DataBaseManagerTest, Put)
{
  Logging::Logger_var logger(
    new Logging::OStream::Logger(
      Logging::OStream::Config(
        std::cerr,
        Logging::Logger::ERROR)));

  Config config;
  config.io_uring_flags = 0;
  config.io_uring_size = 10;
  DataBaseManager data_base_manager(config, logger.in());

  std::string key1 = "key1";
  std::string value1 = "value1";
  std::string key2 = "key2";
  std::string value2 = "value2";

  {
    const std::string column_family_name = "column";
    auto data_base = create_rocksdb(
      column_family_name,
      logger.in());
    auto& column_family_handle = data_base->column_family(
      column_family_name);

    rocksdb::WriteOptions write_options;
    write_options.disableWAL = true;

    std::promise<void> promise;
    auto future = promise.get_future();
    data_base_manager.put(
      *data_base,
      column_family_handle,
      write_options,
      key1,
      value1,
      [promise = std::move(promise)] (const rocksdb::Status& status) mutable {
        promise.set_value();
      });
    future.wait();

    auto status = data_base_manager.put(
      *data_base,
      column_family_handle,
      write_options,
      key2,
      value2);
    EXPECT_TRUE(status.ok());

    rocksdb::FlushOptions flush_options;
    flush_options.wait = true;
    flush_options.allow_write_stall = false;
    status = data_base->get().Flush(
      flush_options,
      &column_family_handle);
    EXPECT_TRUE(status.ok());
  }

  {
    const std::string column_family_name = "column";
    auto data_base = create_rocksdb(
      column_family_name,
      logger.in(),
      false);
    auto& column_family_handle = data_base->column_family(
      column_family_name);
    std::string result;
    auto status = data_base->get().Get(
      {},
      &column_family_handle,
      key1,
      &result);
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(result, value1);

    status = data_base->get().Get(
      {},
      &column_family_handle,
      key2,
      &result);
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(result, value2);
  }
}

TEST(DataBaseManagerTest, Pool)
{
  Logging::Logger_var logger(
    new Logging::OStream::Logger(
      Logging::OStream::Config(
        std::cerr,
        Logging::Logger::ERROR)));

  Config config;
  config.io_uring_size = 10;
  config.number_io_urings = 3;
  config.event_queue_max_size = 10000;
  config.io_uring_flags = IORING_SETUP_ATTACH_WQ;
  DataBaseManagerPool pool(config, logger.in());

  const std::string column_family_name = "column";
  auto data_base = create_rocksdb(
    column_family_name,
    logger.in());
  auto& column_family_handle = data_base->column_family(
    column_family_name);

  const std::string key = "key";
  const std::string value = "value";

  rocksdb::WriteOptions write_options;
  write_options.disableWAL = true;

  const std::size_t count = 10000;

  for (std::size_t i = 1; i <= count; ++i)
  {
    auto status = pool.put(
      *data_base,
      column_family_handle,
      write_options,
      key + std::to_string(i),
      value + std::to_string(i));
    EXPECT_TRUE(status.ok());
  }

  for (std::size_t i = 1; i <= count; ++i)
  {
    std::string result;
    auto status = pool.get(
      *data_base,
      column_family_handle,
      rocksdb::ReadOptions{},
      key + std::to_string(i),
      result);
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(result, value + std::to_string(i));
  }
}