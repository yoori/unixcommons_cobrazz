// GTEST
#include <gtest/gtest.h>

// STD
#include <future>
#include <fstream>
#include <sstream>
#include <streambuf>
#include <string>
#include <vector>

// THIS
#include <Logger/StreamLogger.hpp>
#include <UServerUtils/Grpc/FileManager/File.hpp>
#include <UServerUtils/Grpc/FileManager/FileManager.hpp>
#include <UServerUtils/Grpc/FileManager/FileManagerPool.hpp>
#include <UServerUtils/Grpc/FileManager/Semaphore.hpp>
#include <UServerUtils/Grpc/FileManager/Utils.hpp>

using namespace UServerUtils::Grpc::FileManager;

namespace
{

void write_file(
  const std::string& path,
  const std::string& data)
{
  std::ofstream stream(path, std::ios::out | std::ios::trunc);
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

void remove_file(const std::string& path)
{
  std::remove(path.c_str());
}

std::string read_file(const std::string& path)
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
           << Utils::safe_strerror(errno)
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

TEST(FileManagerTest, Semaphore)
{
  {
    Semaphore semaphore(true, 0);
    EXPECT_TRUE(semaphore.add());
    EXPECT_TRUE(semaphore.add());

    EXPECT_TRUE(semaphore.fetch());
    EXPECT_TRUE(semaphore.fetch());
    EXPECT_FALSE(semaphore.fetch());
    EXPECT_FALSE(semaphore.fetch());
    EXPECT_FALSE(semaphore.fetch());

    EXPECT_TRUE(semaphore.add());
    EXPECT_TRUE(semaphore.fetch());
    EXPECT_FALSE(semaphore.fetch());
  }

  {
    Semaphore semaphore(true, 3);
    EXPECT_TRUE(semaphore.fetch());
    EXPECT_TRUE(semaphore.fetch());
    EXPECT_TRUE(semaphore.fetch());
    EXPECT_FALSE(semaphore.fetch());
  }

  {
    Semaphore semaphore(false, 3);
    EXPECT_TRUE(semaphore.fetch());
    EXPECT_TRUE(semaphore.fetch());
    EXPECT_TRUE(semaphore.fetch());
    boost::scoped_thread<
      boost::join_if_joinable,
      std::thread> th([&semaphore] () {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        semaphore.add();
    });
    EXPECT_TRUE(semaphore.fetch());
  }
}

TEST(FileManagerTest, FileManagerDestroy)
{
  Logging::Logger_var logger(
    new Logging::OStream::Logger(
      Logging::OStream::Config(
          std::cout)));
  for (int i = 0; i <= 100; ++i)
  {
    Config config;
    config.io_uring_size = 10;
    FileManager file_manager(config, logger.in());
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
    File file1(path);
    File file2(file1);
    EXPECT_EQ(file1.get_length(), file2.get_length());

    const auto info = file1.get_info();
    EXPECT_TRUE(info);
  }

  {
    File file1(path);
    File file2(std::move(file1));
    EXPECT_FALSE(file1.is_valid());
    EXPECT_TRUE(file2.is_valid());
    EXPECT_EQ(file2.get_length(), data.size());
  }

  {
    File file(path);
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
  using WriteCallback = FileManager::Callback;

  const std::string directory = "/tmp/";
  const std::string file_name = "test_file_write";

  Logging::Logger_var logger(
    new Logging::OStream::Logger(
      Logging::OStream::Config(
        std::cout)));

  Config config;
  config.io_uring_size = 10;
  config.event_queue_max_size = 10000;

  FileManager file_manager(config, logger.in());

  {
    const std::string path = directory + file_name;
    const std::string data("qwerty");
    remove_file(path);

    File file(path, O_CREAT | O_RDWR | O_APPEND);
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

    File file(path, O_CREAT | O_RDWR | O_APPEND);
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
    File file;
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

    File file(path, O_CREAT | O_RDWR);
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
    const std::size_t count_write = 1000;

    std::vector<File> files;
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
  using ReadCallback = FileManager::Callback;

  const std::string directory = "/tmp/";
  const std::string file_name = "test_file_read";
  const std::string path_to_file = directory + file_name;
  const std::string data = "123456789";

  remove_file(path_to_file);
  write_file(directory + file_name, data);

  Logging::Logger_var logger(
    new Logging::OStream::Logger(
      Logging::OStream::Config(
        std::cout)));

  Config config;
  config.io_uring_size = 10;
  config.event_queue_max_size = 10000;

  FileManager file_manager(config, logger.in());

  {
    File file(path_to_file, O_RDONLY);
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
    File file(path_to_file, O_RDONLY);
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
    File file(path_to_file, O_RDONLY);
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
    File file;
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
    File file(path_to_file, O_RDONLY);
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
        std::cout)));

  Config config;
  config.io_uring_size = 10;
  config.event_queue_max_size = 10000;

  FileManager file_manager(config, logger.in());

  {
    std::string buffer;
    buffer.resize(data.size());
    File file(path_to_file, O_RDONLY);
    const int result = file_manager.read(file, buffer, 0);
    EXPECT_EQ(result, data.size());
    EXPECT_EQ(buffer, data);
  }

  {
    std::string buffer;
    buffer.resize(data.size());
    File file;
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
        std::cout)));

  Config config;
  config.io_uring_size = 10;
  config.event_queue_max_size = 10000;

  FileManager file_manager(config, logger.in());

  {
    File file(path, O_CREAT | O_RDWR | O_APPEND);
    EXPECT_TRUE(file.is_valid());
    const int result = file_manager.write(file, data, -1);
    EXPECT_EQ(result, data.size());
    file.close();
    EXPECT_EQ(data, read_file(path));
  }

  {
    File file;
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
        std::cout)));

  Config config;
  config.io_uring_size = 10;
  config.event_queue_max_size = 10000;
  config.number_io_urings = 2;

  FileManagerPool file_manager_pool(config, logger.in());

  {
    for (std::size_t i = 1; i <= 100; ++i)
    {
      File file(path, O_CREAT | O_RDWR | O_APPEND);
      EXPECT_TRUE(file.is_valid());
      const int result = file_manager_pool.write(file, data, -1);
      EXPECT_EQ(result, data.size());
      file.close();
      EXPECT_EQ(data, read_file(path));
      remove_file(path);
    }
  }
}