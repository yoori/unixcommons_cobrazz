// GTEST
#include <gtest/gtest.h>

// STD
#include <future>
#include <fstream>
#include <sstream>
#include <streambuf>
#include <string>
#include <vector>
#include <coroutine>

// THIS
#include <Logger/StreamLogger.hpp>
#include <UServerUtils/FileManager/File.hpp>
#include <UServerUtils/FileManager/FileManager.hpp>
#include <UServerUtils/FileManager/FileManagerPool.hpp>
#include <UServerUtils/FileManager/Semaphore.hpp>
#include <UServerUtils/FileManager/Utils.hpp>

namespace
{

[[maybe_unused]] void write_file(
  const std::string& path,
  const std::string& data)
{
  std::ofstream stream(path, std::ios::trunc);
  if (!stream)
  {
    std::ostringstream stream;
    stream << FNS
           << "Can't open file="
           << path;
    throw std::runtime_error(stream.str());
  }

  stream << data;
  if (!stream)
  {
    std::ostringstream stream;
    stream << FNS
           << "Writing to file="
           << path
           << " is failed";
    throw std::runtime_error(stream.str());
  }
}

[[maybe_unused]] void remove_file(const std::string& path)
{
  std::remove(path.c_str());
}

[[maybe_unused]] std::string read_file(const std::string& path)
{
  std::ifstream stream(path);
  if (!stream)
  {
    std::ostringstream stream;
    stream << FNS
           << "Can't open file="
           << path
           << " reason=[code="
           << errno
           << ", message="
           << UServerUtils::FileManager::Utils::safe_strerror(errno)
           << "]";
    throw std::runtime_error(stream.str());
  }

  std::string result(
    (std::istreambuf_iterator<char>(stream)),
    std::istreambuf_iterator<char>());
  if (!stream)
  {
    std::ostringstream stream;
    stream << FNS
           << "Can't read file="
           << path;
    throw std::runtime_error(stream.str());
  }

  return result;
}

} // namespace

TEST(FileManagerTest, IoUring)
{
  {
    UServerUtils::FileManager::Config config;
    config.io_uring_flags = 0;
    UServerUtils::FileManager::IoUring uring(config);
    EXPECT_TRUE(uring.get() != nullptr);
    EXPECT_EQ(config.io_uring_flags, uring.params().flags);
  }

  {
    UServerUtils::FileManager::Config config;
    config.io_uring_flags = IORING_SETUP_ATTACH_WQ;
    EXPECT_THROW(UServerUtils::FileManager::IoUring{config}, eh::Exception);
  }

  {
    UServerUtils::FileManager::Config config;
    config.io_uring_flags = 0;
    UServerUtils::FileManager::IoUring uring(config);

    config.io_uring_flags = IORING_SETUP_ATTACH_WQ;
    UServerUtils::FileManager::IoUring uring2(config, uring.get()->ring_fd);
    EXPECT_TRUE(uring.get() != nullptr);
  }
}

TEST(FileManagerTest, Semaphore)
{
  {
    UServerUtils::FileManager::Semaphore semaphore(
      UServerUtils::FileManager::Semaphore::Type::NonBlocking,
      0);
    EXPECT_TRUE(semaphore.add());
    EXPECT_TRUE(semaphore.add());

    EXPECT_TRUE(semaphore.consume());
    EXPECT_TRUE(semaphore.consume());
    EXPECT_FALSE(semaphore.consume());
    EXPECT_FALSE(semaphore.consume());
    EXPECT_FALSE(semaphore.consume());

    EXPECT_TRUE(semaphore.add());
    EXPECT_TRUE(semaphore.consume());
    EXPECT_FALSE(semaphore.consume());

    EXPECT_TRUE(semaphore.add());
    EXPECT_TRUE(semaphore.add());
    EXPECT_TRUE(semaphore.add());
    EXPECT_EQ(semaphore.try_consume(3), 3);

    EXPECT_TRUE(semaphore.add());
    EXPECT_TRUE(semaphore.add());
    EXPECT_TRUE(semaphore.add());
    EXPECT_TRUE(semaphore.add());
    EXPECT_EQ(semaphore.try_consume(3), 3);
    EXPECT_TRUE(semaphore.consume());

    EXPECT_TRUE(semaphore.add());
    EXPECT_TRUE(semaphore.add());
    EXPECT_TRUE(semaphore.add());
    EXPECT_EQ(semaphore.try_consume(4), 3);
  }

  {
    UServerUtils::FileManager::Semaphore semaphore(
      UServerUtils::FileManager::Semaphore::Type::NonBlocking,
      3);
    EXPECT_TRUE(semaphore.consume());
    EXPECT_TRUE(semaphore.consume());
    EXPECT_TRUE(semaphore.consume());
    EXPECT_FALSE(semaphore.consume());
  }

  {
    UServerUtils::FileManager::Semaphore semaphore(
      UServerUtils::FileManager::Semaphore::Type::Blocking,
      3);
    EXPECT_TRUE(semaphore.consume());
    EXPECT_TRUE(semaphore.consume());
    EXPECT_TRUE(semaphore.consume());
    boost::scoped_thread<
      boost::join_if_joinable,
      std::thread> th([&semaphore] () {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        semaphore.add();
    });
    EXPECT_TRUE(semaphore.consume());

    EXPECT_TRUE(semaphore.add());
    EXPECT_TRUE(semaphore.add());
    EXPECT_TRUE(semaphore.add());
    EXPECT_EQ(semaphore.try_consume(3), 3);

    EXPECT_TRUE(semaphore.add());
    EXPECT_TRUE(semaphore.add());
    EXPECT_TRUE(semaphore.add());
    EXPECT_TRUE(semaphore.add());
    EXPECT_EQ(semaphore.try_consume(3), 3);
    EXPECT_TRUE(semaphore.consume());

    EXPECT_TRUE(semaphore.add());
    EXPECT_TRUE(semaphore.add());
    EXPECT_TRUE(semaphore.add());
    EXPECT_EQ(semaphore.try_consume(4), 3);
  }
}

TEST(FileManagerTest, FileManagerDestroy)
{
  Logging::Logger_var logger(
    new Logging::OStream::Logger(
      Logging::OStream::Config(
          std::cerr,
          Logging::Logger::CRITICAL)));
  for (int i = 0; i <= 100; ++i)
  {
    UServerUtils::FileManager::Config config;
    config.io_uring_flags = 0;
    config.io_uring_size = 10;
    UServerUtils::FileManager::FileManager file_manager(config, logger.in());
  }

  EXPECT_TRUE(true);
}

TEST(FileManagerTest, File)
{
  const std::string path("/tmp/test.txt");
  const std::string data("12345");
  remove_file(path);
  write_file(path, data);

  {
    UServerUtils::FileManager::File file1(path);
    UServerUtils::FileManager::File file2(file1);
    EXPECT_EQ(file1.get_length(), file2.get_length());

    const auto info = file1.get_info();
    EXPECT_TRUE(info);
  }

  {
    UServerUtils::FileManager::File file1(path);
    UServerUtils::FileManager::File file2(std::move(file1));
    EXPECT_FALSE(file1.is_valid());
    EXPECT_TRUE(file2.is_valid());
    EXPECT_EQ(file2.get_length(), data.size());
  }

  {
    UServerUtils::FileManager::File file(path);
    EXPECT_TRUE(file.is_valid());
    EXPECT_EQ(file.get_length(), data.size());

    const int new_size = 1000;
    EXPECT_TRUE(file.set_length(new_size));
    EXPECT_EQ(file.get_length(), new_size);

    file.close();
    EXPECT_FALSE(file.is_valid());
  }

  remove_file(path);
}

TEST(FileManagerTest, WriteCallback)
{
  using WriteCallback = UServerUtils::FileManager::FileManager::Callback;

  const std::string directory = "/tmp/";
  const std::string file_name = "test_file_write";

  Logging::Logger_var logger(
    new Logging::OStream::Logger(
      Logging::OStream::Config(
        std::cerr,
        Logging::Logger::CRITICAL)));

  UServerUtils::FileManager::Config config;
  config.io_uring_flags = 0;
  config.io_uring_size = 10;
  config.event_queue_max_size = 10000;

  UServerUtils::FileManager::FileManager file_manager(
    config,
    logger.in());

  {
    const std::string path = directory + file_name;
    const std::string data("qwerty");
    remove_file(path);

    UServerUtils::FileManager::File file(path, O_CREAT | O_RDWR | O_APPEND);
    EXPECT_TRUE(file.is_valid());

    const std::size_t count_write = 10;
    for (std::size_t i = 1; i <= count_write; ++i)
    {
      std::promise<int> promise;
      auto future = promise.get_future();
      WriteCallback write_callback(
        [promise = std::move(promise)] (const int result) mutable {
          promise.set_value(result);
      });

      file_manager.write(file, data, -1, std::move(write_callback));

      const auto count = future.get();
      EXPECT_EQ(count, data.size());
    }

    std::string required_data;
    required_data.reserve(count_write * data.size());
    for (std::size_t i = 1; i <= count_write; ++i)
    {
      required_data += data;
    }

    file.close();
    EXPECT_EQ(required_data, read_file(path));
    remove_file(path);
  }

  {
    const std::string path = directory + file_name;
    const std::string data("qwerty");
    remove_file(path);

    UServerUtils::FileManager::File file(path, O_CREAT | O_RDWR | O_APPEND);
    EXPECT_TRUE(file.is_valid());

    const std::size_t count_write = 10000;
    for (std::size_t i = 1; i <= count_write; ++i)
    {
      std::promise<int> promise;
      auto future = promise.get_future();
      WriteCallback write_callback(
        [promise = std::move(promise)] (const int result) mutable {
          promise.set_value(result);
      });

      file_manager.write(file, data, -1, std::move(write_callback));

      const auto count = future.get();
      EXPECT_EQ(count, data.size());
    }
    const auto file_size = file.get_length();
    EXPECT_TRUE(file_size);
    if (file_size)
    {
      EXPECT_EQ(count_write * data.size(), *file.get_length());
    }

    remove_file(path);
  }

  {
    const std::string data("qwerty");
    UServerUtils::FileManager::File file;
    EXPECT_FALSE(file.is_valid());

    std::promise<int> promise;
    auto future = promise.get_future();
    WriteCallback write_callback(
      [promise = std::move(promise)] (const int result) mutable {
        promise.set_value(result);
    });

    file_manager.write(file, data, -1, std::move(write_callback));
    EXPECT_EQ(future.get(), -EBADF);
  }

  {
    const std::string path = directory + file_name;
    const std::string data("qwerty");
    remove_file(path);

    UServerUtils::FileManager::File file(path, O_CREAT | O_RDWR);
    EXPECT_TRUE(file.is_valid());

    const std::size_t count_write = 10000;
    const std::int64_t offset = 1000;
    for (std::size_t i = 1; i <= count_write; ++i)
    {
      std::promise<int> promise;
      auto future = promise.get_future();
      WriteCallback write_callback(
        [promise = std::move(promise)] (const int result) mutable {
          promise.set_value(result);
      });

      file_manager.write(file, data, offset, std::move(write_callback));

      const auto count = future.get();
      EXPECT_EQ(count, data.size());
    }
    const auto file_size = file.get_length();
    EXPECT_TRUE(file_size);
    if (file_size)
    {
      EXPECT_EQ(offset + data.size(), *file.get_length());
    }

    remove_file(path);
  }

  {
    const std::string data("qwerty");
    const std::size_t count_write = 3000;

    std::vector<UServerUtils::FileManager::File> files;
    files.reserve(count_write);
    for (std::size_t i = 1; i <= count_write; ++i)
    {
      const std::string path =
        directory + file_name + std::to_string(i);
      remove_file(path);
      files.emplace_back(path, O_CREAT | O_RDWR | O_APPEND);
      EXPECT_TRUE(files.back().is_valid());
    }

    std::vector<std::future<int>> futures;
    futures.reserve(count_write);
    for (std::size_t i = 0; i < count_write; ++i)
    {
      std::promise<int> promise;
      futures.emplace_back(promise.get_future());
      WriteCallback write_callback(
        [promise = std::move(promise)] (const int result) mutable {
          promise.set_value(result);
      });

      file_manager.write(files[i], data, 0, std::move(write_callback));
    }

    for (auto& future : futures)
    {
      EXPECT_EQ(future.get(), data.size());
    }

    for (const auto& file : files)
    {
      const auto length = file.get_length();
      EXPECT_TRUE(length);
      EXPECT_EQ(length, data.size());
    }

    for (std::size_t i = 1; i <= count_write; ++i)
    {
      const std::string path =
        directory + file_name + std::to_string(i);
      remove_file(path);
    }
  }
}

TEST(FileManagerTest, ReadCallback)
{
  using ReadCallback = UServerUtils::FileManager::FileManager::Callback;

  const std::string directory = "/tmp/";
  const std::string file_name = "test_file_read";
  const std::string path_to_file = directory + file_name;
  const std::string data = "123456789";

  remove_file(path_to_file);
  write_file(directory + file_name, data);

  Logging::Logger_var logger(
    new Logging::OStream::Logger(
      Logging::OStream::Config(
        std::cerr,
        Logging::Logger::CRITICAL)));

  UServerUtils::FileManager::Config config;
  config.io_uring_flags = 0;
  config.io_uring_size = 10;
  config.event_queue_max_size = 10000;

  UServerUtils::FileManager::FileManager file_manager(
    config,
    logger.in());

  {
    UServerUtils::FileManager::File file(path_to_file, O_RDONLY);
    EXPECT_TRUE(file.is_valid());

    for (std::size_t i = 1; i <= 1000; ++i)
    {
      std::promise<int> promise;
      auto future = promise.get_future();
      ReadCallback read_callback(
        [promise = std::move(promise)] (const int result) mutable {
          promise.set_value(result);
      });

      const std::size_t buffer_size = 5;
      EXPECT_TRUE(buffer_size < data.size());
      std::string buffer;
      buffer.resize(buffer_size);
      file_manager.read(file, buffer, 0, std::move(read_callback));
      EXPECT_EQ(future.get(), buffer_size);
      EXPECT_EQ(buffer, data.substr(0, buffer_size));
    }
  }

  {
    const std::size_t number = 3000;
    std::atomic<std::size_t> count{0};
    UServerUtils::FileManager::File file(path_to_file, O_RDONLY);
    EXPECT_TRUE(file.is_valid());
    {
      UServerUtils::FileManager::FileManager file_manager2(
        config,
        logger.in());
      const std::size_t buffer_size = 5;
      for (std::size_t i = 1; i <= number; ++i)
      {
        EXPECT_TRUE(buffer_size < data.size());
        std::string buffer;
        buffer.resize(buffer_size);
        std::string_view buffer_view(buffer);
        ReadCallback read_callback(
          [&count, &buffer_size, buffer = std::move(buffer)] (const int result) mutable {
            count.fetch_add(1, std::memory_order_relaxed);
            EXPECT_EQ(buffer_size, result);
          });

        file_manager2.read(file, buffer_view, 0, std::move(read_callback));
      }
    }
    EXPECT_EQ(number, count.load());
  }

  {
    UServerUtils::FileManager::File file(path_to_file, O_RDONLY);
    EXPECT_TRUE(file.is_valid());

    for (std::size_t i = 1; i <= 1000; ++i)
    {
      std::promise<int> promise;
      auto future = promise.get_future();
      ReadCallback read_callback(
        [promise = std::move(promise)] (const int result) mutable {
          promise.set_value(result);
      });

      const std::size_t buffer_size = 50;
      EXPECT_TRUE(buffer_size > data.size());
      std::string buffer;
      buffer.resize(buffer_size);
      file_manager.read(file, buffer, 0, std::move(read_callback));
      const int result = future.get();
      EXPECT_EQ(result, data.size());
      buffer.resize(result);
      EXPECT_EQ(buffer, data);
    }
  }

  {
    UServerUtils::FileManager::File file(path_to_file, O_RDONLY);
    EXPECT_TRUE(file.is_valid());

    std::promise<int> promise;
    auto future = promise.get_future();
    ReadCallback read_callback(
      [promise = std::move(promise)] (const int result) mutable {
        promise.set_value(result);
    });

    const std::size_t buffer_size = data.size();
    std::string buffer;
    buffer.resize(buffer_size);
    file_manager.read(file, buffer, 1, std::move(read_callback));
    EXPECT_EQ(future.get(), buffer_size - 1);
    buffer.resize(buffer_size - 1);
    EXPECT_EQ(buffer, data.substr(1, buffer_size - 1));
  }

  {
    UServerUtils::FileManager::File file;
    EXPECT_FALSE(file.is_valid());

    std::promise<int> promise;
    auto future = promise.get_future();
    ReadCallback read_callback(
      [promise = std::move(promise)] (const int result) mutable {
        promise.set_value(result);
    });
    std::string buffer;
    buffer.resize(100);
    file_manager.read(file, buffer, 0, std::move(read_callback));
    EXPECT_EQ(future.get(), -EBADF);
  }

  {
    UServerUtils::FileManager::File file(path_to_file, O_RDONLY);
    EXPECT_TRUE(file.is_valid());

    std::promise<int> promise;
    auto future = promise.get_future();
    ReadCallback read_callback(
      [promise = std::move(promise)] (const int result) mutable {
        promise.set_value(result);
    });
    std::string buffer;
    buffer.resize(100);
    file_manager.read(file, buffer, data.size(), std::move(read_callback));
    EXPECT_EQ(future.get(), 0);
  }
}

TEST(FileManagerTest, ReadBlockThread)
{
  const std::string directory = "/tmp/";
  const std::string file_name = "test_file_read";
  const std::string path_to_file = directory + file_name;
  const std::string data = "123456789";

  remove_file(path_to_file);
  write_file(directory + file_name, data);

  Logging::Logger_var logger(
    new Logging::OStream::Logger(
      Logging::OStream::Config(
        std::cerr,
        Logging::Logger::CRITICAL)));

  UServerUtils::FileManager::Config config;
  config.io_uring_flags = 0;
  config.io_uring_size = 10;
  config.event_queue_max_size = 10000;

  UServerUtils::FileManager::FileManager file_manager(
    config,
    logger.in());

  {
    std::string buffer;
    buffer.resize(data.size());
    UServerUtils::FileManager::File file(path_to_file, O_RDONLY);
    const int result = file_manager.read(file, buffer, 0);
    EXPECT_EQ(result, data.size());
    EXPECT_EQ(buffer, data);
  }

  {
    std::string buffer;
    buffer.resize(data.size());
    UServerUtils::FileManager::File file;
    const int result = file_manager.read(file, buffer, 0);
    EXPECT_EQ(result, -EBADF);
  }
}

TEST(FileManagerTest, WriteBlockThread)
{
  const std::string directory = "/tmp/";
  const std::string file_name = "test_file_write";
  const std::string path = directory + file_name;
  const std::string data("qwerty");
  remove_file(path);

  Logging::Logger_var logger(
    new Logging::OStream::Logger(
      Logging::OStream::Config(
        std::cerr,
        Logging::Logger::CRITICAL)));

  UServerUtils::FileManager::Config config;
  config.io_uring_flags = 0;
  config.io_uring_size = 10;
  config.event_queue_max_size = 10000;

  UServerUtils::FileManager::FileManager file_manager(
    config,
    logger.in());

  {
    UServerUtils::FileManager::File file(path, O_CREAT | O_RDWR | O_APPEND);
    EXPECT_TRUE(file.is_valid());
    const int result = file_manager.write(file, data, -1);
    EXPECT_EQ(result, data.size());
    file.close();
    EXPECT_EQ(data, read_file(path));
  }

  {
    UServerUtils::FileManager::File file;
    EXPECT_FALSE(file.is_valid());
    const int result = file_manager.write(file, data, -1);
    EXPECT_EQ(result, -EBADF);
  }
}

TEST(FileManagerTest, FileManagerPool)
{
  const std::string directory = "/tmp/";
  const std::string file_name = "test_file_write";
  const std::string path = directory + file_name;
  const std::string data("qwerty");
  remove_file(path);

  Logging::Logger_var logger(
    new Logging::OStream::Logger(
      Logging::OStream::Config(
        std::cerr,
        Logging::Logger::CRITICAL)));

  UServerUtils::FileManager::Config config;
  config.io_uring_flags = 0;
  config.io_uring_size = 10;
  config.event_queue_max_size = 10000;
  config.number_io_urings = 2;

  UServerUtils::FileManager::FileManagerPool file_manager_pool(
    config,
    logger.in());

  {
    for (std::size_t i = 1; i <= 100; ++i)
    {
      UServerUtils::FileManager::File file(path, O_CREAT | O_RDWR | O_APPEND);
      EXPECT_TRUE(file.is_valid());
      const int result = file_manager_pool.write(file, data, -1);
      EXPECT_EQ(result, data.size());
      file.close();
      EXPECT_EQ(data, read_file(path));
      remove_file(path);
    }
  }
}

TEST(FileManagerTest, Flag_IORING_SETUP_ATTACH_WQ)
{
  const std::string directory = "/tmp/";
  const std::string file_name = "test_file_write";
  const std::string path = directory + file_name;
  const std::string data("qwerty");
  remove_file(path);

  Logging::Logger_var logger(
    new Logging::OStream::Logger(
      Logging::OStream::Config(
        std::cerr,
        Logging::Logger::CRITICAL)));

  UServerUtils::FileManager::Config config;
  config.io_uring_size = 10;
  config.event_queue_max_size = 10000;
  config.number_io_urings = 2;
  config.io_uring_flags = IORING_SETUP_ATTACH_WQ;

  UServerUtils::FileManager::FileManagerPool file_manager_pool(config, logger.in());

  {
    for (std::size_t i = 1; i <= 100; ++i)
    {
      UServerUtils::FileManager::File file(path, O_CREAT | O_RDWR | O_APPEND);
      EXPECT_TRUE(file.is_valid());
      const int result = file_manager_pool.write(file, data, -1);
      EXPECT_EQ(result, data.size());
      file.close();
      EXPECT_EQ(data, read_file(path));
      remove_file(path);
    }
  }
}