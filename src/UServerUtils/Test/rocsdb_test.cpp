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
#include <UServerUtils/RocksDB/DataBase.hpp>
#include <UServerUtils/RocksDB/DataBaseManager.hpp>
#include <UServerUtils/RocksDB/DataBaseManagerPool.hpp>

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

[[maybe_unused]] std::shared_ptr<DataBase> create_rocksdb(
  const std::string& column_family_name,
  Logging::Logger* logger,
  const bool need_recreate = true,
  const std::optional<std::int32_t>& ttl = {})
{
  const std::string path_db = "/tmp/rocks_db";
  if (need_recreate)
  {
    remove_directory(path_db);
  }

  auto db_options = create_db_options();
  db_options.create_if_missing = true;
  auto column_family_options = create_column_family_options();

  std::optional<std::vector<std::int32_t>> ttls;
  if (ttl)
  {
    ttls = std::vector<std::int32_t>{*ttl};
  }

  std::vector<rocksdb::ColumnFamilyDescriptor> descriptors{
    {column_family_name, column_family_options}};
  return std::make_shared<DataBase>(
    logger,
    path_db,
    db_options,
    descriptors,
    true,
    ttls);
}

} // namespace


void test_DataBase(std::optional<std::int32_t> ttl)
{
  Logging::Logger_var logger(
    new Logging::OStream::Logger(
      Logging::OStream::Config(
        std::cerr,
        Logging::Logger::CRITICAL)));

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
      false,
      std::vector<std::int32_t>(1, *ttl)), eh::Exception);
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
      true,
      std::vector<std::int32_t>(1, *ttl)), eh::Exception);
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
        true,
        std::vector<std::int32_t>(1, *ttl));

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
        false,
        std::vector<std::int32_t>(1, *ttl));

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
        false,
        std::vector<std::int32_t>(2, *ttl)), eh::Exception);
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
        true,
        std::vector<std::int32_t>(2, *ttl));
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
        false,
        std::vector<std::int32_t>(2, *ttl));
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
        false,
        std::vector<std::int32_t>(1, *ttl)), eh::Exception);
    }
  }
}

TEST(ROCKSDB, DataBase)
{
  test_DataBase({});
  test_DataBase(5);
}

TEST(DataBaseManagerTest, DataBaseManagerDestroy)
{
  Logging::Logger_var logger(
    new Logging::OStream::Logger(
      Logging::OStream::Config(
        std::cerr,
        Logging::Logger::CRITICAL)));
  for (int i = 0; i <= 100; ++i)
  {
    Config config;
    config.io_uring_flags = 0;
    config.io_uring_size = 10;
    DataBaseManager data_base_manager(config, logger.in());
  }

  EXPECT_TRUE(true);
}

void test_Get(std::optional<std::int32_t> ttl)
{
  Logging::Logger_var logger(
    new Logging::OStream::Logger(
      Logging::OStream::Config(
        std::cerr,
        Logging::Logger::CRITICAL)));

  const std::string column_family_name = "column";

  Config config;
  config.io_uring_flags = 0;
  config.io_uring_size = 4096 * 4;
  config.event_queue_max_size = 1000000;
  std::unique_ptr<DataBaseManager> data_base_manager =
    std::make_unique<DataBaseManager>(config, logger.in());

  const std::string key = "key";
  const std::string value = "value";

  {
    auto data_base = create_rocksdb(column_family_name, logger.in(), true, ttl);
    auto& column_family_handle = data_base->column_family(column_family_name);

    std::promise<void> promise;
    auto future = promise.get_future();
    data_base_manager->get(
      data_base,
      column_family_handle,
      rocksdb::ReadOptions{},
      key,
      [promise = std::move(promise)] (
        rocksdb::Status&& status,
        const std::string_view value) mutable {
          EXPECT_EQ(status.code(), rocksdb::Status::Code::kNotFound);
          promise.set_value();
      });
    future.wait();
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(700));

  {
    auto data_base = create_rocksdb(column_family_name, logger.in(), false, ttl);
    auto& column_family_handle = data_base->column_family(column_family_name);

    rocksdb::WriteOptions write_options;
    write_options.sync = true;
    write_options.disableWAL = false;
    const auto status = data_base_manager->put(
      data_base,
      column_family_handle,
      write_options,
      key,
      value);
    EXPECT_TRUE(status.ok());
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(700));

  {
    auto data_base = create_rocksdb(column_family_name, logger.in(), false, ttl);
    auto& column_family_handle = data_base->column_family(column_family_name);

    std::promise<void> promise;
    auto future = promise.get_future();
    data_base_manager->get(
      data_base,
      column_family_handle,
      rocksdb::ReadOptions{},
      key,
      [promise = std::move(promise), value] (
        rocksdb::Status&& status,
        const std::string_view result) mutable {
          EXPECT_TRUE(status.ok());
          EXPECT_EQ(value, result);
          promise.set_value();
      });
    future.wait();
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(700));

  {
    auto data_base = create_rocksdb(column_family_name, logger.in(), false, ttl);
    auto& column_family_handle = data_base->column_family(column_family_name);

    std::string result;
    const auto status = data_base_manager->get(
      data_base,
      column_family_handle,
      rocksdb::ReadOptions{},
      key,
      result);
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(result, value);
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(700));
  const std::size_t number = 100000;
  {
    auto data_base = create_rocksdb(column_family_name, logger.in(), false, ttl);
    auto& column_family_handle = data_base->column_family(column_family_name);
    rocksdb::WriteOptions write_options;
    write_options.disableWAL = true;
    for (std::size_t i = 1; i <= number; ++i)
    {
      const std::string key_result = key + std::to_string(i);
      const std::string value_result = value + std::to_string(i);
      const auto status = data_base_manager->put(
        data_base,
        column_family_handle,
        write_options,
        key_result,
        value_result);
      EXPECT_TRUE(status.ok());
    }

      write_options.disableWAL = false;
      for (std::size_t i = number + 1; i <= number * 2; ++i)
      {
        const std::string key_result = key + std::to_string(i);
        const std::string value_result = value + std::to_string(i);
        const auto status = data_base_manager->put(
          data_base,
          column_family_handle,
          write_options,
          key_result,
          value_result);
          EXPECT_TRUE(status.ok());
      }

      write_options.sync = true;
      write_options.disableWAL = false;
      for (std::size_t i = 2 * number + 1; i <= number * 3; ++i)
      {
        const std::string key_result = key + std::to_string(i);
        const std::string value_result = value + std::to_string(i);
        const auto status = data_base_manager->put(
          data_base,
          column_family_handle,
          write_options,
          key_result,
          value_result);
        EXPECT_TRUE(status.ok());
      }

    const auto status  = data_base->get().Flush(rocksdb::FlushOptions{}, &column_family_handle);
    EXPECT_TRUE(status.ok());
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  {
    auto data_base = create_rocksdb(column_family_name, logger.in(), false, ttl);
    auto& column_family_handle = data_base->column_family(column_family_name);
    std::atomic<std::size_t> count{0};
    {
      for (std::size_t i = 1; i <= 3 * number; ++i)
      {
        auto key_result = std::make_unique<std::string>(
          key + std::to_string(i));
        std::string_view key_result_view(*key_result);
        data_base_manager->get(
          data_base,
          column_family_handle,
          rocksdb::ReadOptions{},
          key_result_view,
          [&count, value, i, key = std::move(key_result)] (
            rocksdb::Status&& status,
            const std::string_view result) mutable {
              EXPECT_TRUE(status.ok());
              EXPECT_EQ(value + std::to_string(i), result);
              count.fetch_add(1, std::memory_order_relaxed);
          });
      }
    }

    data_base_manager.reset();
    EXPECT_EQ(3 * number, count.load());
  }

  data_base_manager = std::make_unique<DataBaseManager>(
    config,
    logger.in());
  const std::string key_delete1 = "key1";
  const std::string key_delete2 = "key2";
  const std::string key_existing = "key3";
  const std::string value_existing = "value3";
  {
    auto data_base = create_rocksdb(column_family_name, logger.in(), false, ttl);
    auto& column_family_handle = data_base->column_family(column_family_name);

    rocksdb::WriteOptions write_options;
    write_options.disableWAL = true;
    auto status = data_base_manager->erase(
      data_base,
      column_family_handle,
      write_options,
      key_delete1);
    EXPECT_TRUE(status.ok());

    write_options.sync = true;
    write_options.disableWAL = false;
    status = data_base_manager->erase(
      data_base,
      column_family_handle,
      write_options,
      key_delete2);
    EXPECT_TRUE(status.ok());
  }

  {
    auto data_base = create_rocksdb(column_family_name, logger.in(), false, ttl);
    auto& column_family_handle = data_base->column_family(column_family_name);

    std::string result;
    auto status = data_base_manager->get(
      data_base,
      column_family_handle,
      rocksdb::ReadOptions{},
      key_delete1,
      result);
    EXPECT_FALSE(status.ok());
    EXPECT_TRUE(status.code() == rocksdb::Status::kNotFound);

    status = data_base_manager->get(
      data_base,
      column_family_handle,
      rocksdb::ReadOptions{},
      key_delete2,
      result);
    EXPECT_FALSE(status.ok());
    EXPECT_TRUE(status.code() == rocksdb::Status::kNotFound);

    status = data_base_manager->get(
      data_base,
      column_family_handle,
      rocksdb::ReadOptions{},
      key_existing,
      result);
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(result, value_existing);
  }
}

TEST(DataBaseManagerTest, Get)
{
  test_Get({});
  test_Get(10000);
}

void test_MultiGet(std::optional<std::int32_t> ttl)
{
  Logging::Logger_var logger(
    new Logging::OStream::Logger(
      Logging::OStream::Config(
        std::cerr,
        Logging::Logger::CRITICAL)));

  const std::string column_family_name = "column";

  Config config;
  config.io_uring_flags = 0;
  config.io_uring_size = 10;
  DataBaseManager data_base_manager(config, logger.in());

  std::string key = "key";
  std::string value = "value";

  {
    auto data_base = create_rocksdb(column_family_name, logger.in(), true, ttl);
    auto& column_family_handle = data_base->column_family(column_family_name);

    rocksdb::WriteOptions write_options;
    write_options.disableWAL = true;

    for (std::size_t i = 1; i <= 100; ++i)
    {
      std::string key_result = key + std::to_string(i);
      std::string value_result = value + std::to_string(i);

      auto status = data_base_manager.put(
        data_base,
        column_family_handle,
        write_options,
        key_result,
        value_result);
      EXPECT_TRUE(status.ok());
    }
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(700));

  {
    auto data_base = create_rocksdb(column_family_name, logger.in(), false, ttl);
    auto& column_family_handle = data_base->column_family(column_family_name);

    const std::string key1 = "key1";
    const std::string value1 = "value1";
    const std::string key2 = "key2";
    const std::string value2 = "value2";
    const std::string key_not_exist = "not exist";

    std::promise<void> promise;
    auto future = promise.get_future();
    data_base_manager.multi_get(
      data_base,
      {&column_family_handle, &column_family_handle, &column_family_handle},
      rocksdb::ReadOptions{},
      {key1, key2, key_not_exist},
      [promise = std::move(promise), &value1, &value2] (
        DataBaseManager::Statuses&& statuses,
        DataBaseManager::Values&& values) {
          EXPECT_EQ(statuses.size(), 3);
          EXPECT_TRUE(statuses[0].ok());
          EXPECT_TRUE(statuses[1].ok());
          EXPECT_FALSE(statuses[2].ok());

          EXPECT_EQ(values.size(), 3);
          EXPECT_EQ(values[0], value1);
          EXPECT_EQ(values[1], value2);
          EXPECT_EQ(values[2], std::string());
      });
    future.wait();
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(700));

  {
    auto data_base = create_rocksdb(column_family_name, logger.in(), false, ttl);
    auto& column_family_handle = data_base->column_family(column_family_name);

    const std::string key1 = "key1";
    const std::string value1 = "value1";
    const std::string key2 = "key2";
    const std::string value2 = "value2";
    const std::string key_not_exist = "not exist";

    std::vector<std::string> values;
    const auto statuses = data_base_manager.multi_get(
      data_base,
      {&column_family_handle, &column_family_handle, &column_family_handle},
      rocksdb::ReadOptions{},
      {key1, key2, key_not_exist},
      values);

    EXPECT_EQ(statuses.size(), 3);
    EXPECT_TRUE(statuses[0].ok());
    EXPECT_TRUE(statuses[1].ok());
    EXPECT_FALSE(statuses[2].ok());

    EXPECT_EQ(values.size(), 3);
    EXPECT_EQ(values[0], value1);
    EXPECT_EQ(values[1], value2);
    EXPECT_EQ(values[2], std::string());
  }
}

TEST(DataBaseManagerTest, MultiGet) {
  test_MultiGet({});
  test_MultiGet(5);
}


void test_Pool(std::optional<std::int32_t> ttl)
{
  Logging::Logger_var logger(
    new Logging::OStream::Logger(
      Logging::OStream::Config(
        std::cerr,
        Logging::Logger::CRITICAL)));

  Config config;
  config.io_uring_size = 10;
  config.number_io_urings = 3;
  config.event_queue_max_size = 10000;
  config.io_uring_flags = IORING_SETUP_ATTACH_WQ;
  DataBaseManagerPool pool(config, logger.in());

  const std::string column_family_name = "column";
  const std::string key = "key";
  const std::string value = "value";
  const std::size_t count = 10000;

  {
    auto data_base = create_rocksdb(column_family_name, logger.in(), true, ttl);
    auto& column_family_handle = data_base->column_family(column_family_name);

    rocksdb::WriteOptions write_options;
    write_options.disableWAL = true;

    for (std::size_t i = 1; i <= count; ++i)
    {
      auto status = pool.put(
        data_base,
        column_family_handle,
        write_options,
        key + std::to_string(i),
        value + std::to_string(i));
      EXPECT_TRUE(status.ok());
    }

    const auto status  = data_base->get().Flush({});
    EXPECT_TRUE(status.ok());
  }

  {
    auto data_base = create_rocksdb(column_family_name, logger.in(), false, ttl);
    auto& column_family_handle = data_base->column_family(column_family_name);

    for (std::size_t i = 1; i <= count; ++i)
    {
      std::string result;
      auto status = pool.get(
        data_base,
        column_family_handle,
        rocksdb::ReadOptions{},
        key + std::to_string(i),
        result);
      EXPECT_TRUE(status.ok());
      EXPECT_EQ(result, value + std::to_string(i));
    }
  }
}

TEST(DataBaseManagerTest, Pool)
{
  test_Pool({});
  test_Pool(5);
}