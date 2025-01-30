// STD
#include <atomic>
#include <fstream>
#include <iostream>
#include <random>

// USERVER
#include <engine/task/task_processor.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/utils/async.hpp>

// THIS
#include <Logger/Logger.hpp>
#include <Logger/StreamLogger.hpp>
#include <UServerUtils/FileManager/Config.hpp>
#include <UServerUtils/FileManager/FileManagerPool.hpp>
#include <UServerUtils/FileManager/Utils.hpp>
#include <UServerUtils/Component.hpp>
#include <UServerUtils/ComponentsBuilder.hpp>
#include <UServerUtils/ComponentsBuilder.hpp>
#include <UServerUtils/Manager.hpp>
#include <UServerUtils/Utils.hpp>

enum class OperationType
{
  Read,
  Write,
  ReadWrite
};

enum class TestType
{
  SingleFile,
  FileToCoro
};

struct Statistics final
{
  Statistics() = default;
  ~Statistics() = default;

  std::atomic<std::uint32_t> success_read{0};
  std::atomic<std::uint32_t> success_write{0};
  std::atomic<std::uint32_t> error_read{0};
  std::atomic<std::uint32_t> error_write{0};
};

class Benchmark final
  : public UServerUtils::Component,
    public ReferenceCounting::AtomicImpl
{
public:
  using FileManagerConfig = UServerUtils::FileManager::Config;
  using Task = userver::engine::TaskWithResult<void>;
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;

public:
  Benchmark(
    const TestType test_type,
    const OperationType operation_type,
    Logger* logger,
    const FileManagerConfig& config,
    const std::string& directory_path,
    const std::string& file_name,
    const std::size_t number_coroutines,
    const std::size_t file_size,
    const std::size_t block_size,
    const bool is_direct,
    Statistics& statistics)
    : test_type_(test_type),
      operation_type_(operation_type),
      logger_(ReferenceCounting::add_ref(logger)),
      directory_path_(directory_path),
      number_coroutines_(number_coroutines),
      file_size_(file_size),
      block_size_(block_size),
      is_direct_(is_direct),
      config_(config),
      file_manager_pool_(config, logger_.in()),
      statistics_(statistics)
  {
  }

protected:
  void activate_object_() override
  {
    auto& current_task_processor = userver::engine::current_task::GetTaskProcessor();
    tasks_.reserve(number_coroutines_);
    for (std::size_t i = 1; i <= number_coroutines_; ++i)
    {
      tasks_.emplace_back(
        userver::utils::Async(
          current_task_processor,
          "file_manager_task",
          &Benchmark::run,
          this,
          i));
    }
  }

  void deactivate_object_() override
  {
    is_strop_ = true;
  }

  void wait_object_() override
  {
    for (auto& task : tasks_)
    {
      try
      {
        if (task.IsValid())
        {
          task.Get();
        }
      }
      catch (const eh::Exception& exc)
      {
        std::ostringstream stream;
        stream << FNS
               << exc.what();
        logger_->error(stream.str());
      }
    }
  }

private:
  void run(const std::size_t number)
  {
    const std::size_t kPageSize = 1024;
    const bool is_iopoll = config_.io_uring_flags & IORING_SETUP_IOPOLL;

    std::string path;
    if (test_type_ == TestType::FileToCoro)
    {
      path = directory_path_ + file_name + std::to_string(number);
      std::remove(path.c_str());
    }
    else
    {
      path = directory_path_ + file_name;
    }

    int flags = O_RDWR;
    if (test_type_ == TestType::FileToCoro)
    {
      flags |= O_CREAT;
    }
    if (is_iopoll || is_direct_)
    {
      flags |= O_DIRECT;
    }
    UServerUtils::FileManager::File file(path, flags);
    if (!file.is_valid())
    {
      std::ostringstream stream;
      stream << FNS
             << "file="
             << path
             << " not valid, reason="
             << file.error_message();
      logger_->emergency(stream.str());
      return;
    }

    std::string read_buffer;
    std::string write_buffer;
    std::unique_ptr<char, UServerUtils::FileManager::Utils::AlignedFreeDeleter> aligned_read_buffer;
    std::unique_ptr<char, UServerUtils::FileManager::Utils::AlignedFreeDeleter> aligned_write_buffer;
    std::string_view read_buffer_view;
    std::string_view write_buffer_view;

    const std::string write_data(block_size_, 'a');
    const std::size_t size_write_buffer = block_size_;
    const std::size_t size_read_buffer = block_size_;
    if (is_iopoll || is_direct_)
    {
      aligned_read_buffer.reset(static_cast<char*>(
          UServerUtils::FileManager::Utils::aligned_alloc(
          size_read_buffer,
          kPageSize)));
      read_buffer_view = std::string_view{
        aligned_read_buffer.get(),
        size_read_buffer};

      aligned_write_buffer.reset(static_cast<char*>(
          UServerUtils::FileManager::Utils::aligned_alloc(
          size_write_buffer,
          kPageSize)));
      std::memcpy(
        aligned_write_buffer.get(),
        write_data.data(),
        size_write_buffer);
      write_buffer_view = std::string_view{
        aligned_write_buffer.get(),
        size_write_buffer};
    }
    else
    {
      read_buffer.resize(size_read_buffer);
      read_buffer_view = read_buffer;
      write_buffer_view = write_data;
    }

    if (test_type_ == TestType::FileToCoro)
    {
      file_manager_pool_.write(file, write_buffer_view, 0);
    }

    const auto length = file.length();
    if (!length)
    {
      std::ostringstream stream;
      stream << FNS
             << "get_length is failed";
      logger_->emergency(stream.str());
      return;
    }

    std::random_device random_device;
    std::mt19937 gen(random_device());
    std::uniform_int_distribution<uint32_t> dist(
      0,
      *length >= block_size_ ? *length - block_size_ : 0);

    while (!is_strop_)
    {
      std::int64_t offset = 0;
      if (test_type_ == TestType::SingleFile)
      {
        if (is_iopoll || is_direct_)
        {
          offset = (dist(gen) / kPageSize) * kPageSize;
        }
        else
        {
          offset = dist(gen);
        }
      }

      if (operation_type_ == OperationType::Write || operation_type_ == OperationType::ReadWrite)
      {
        int result = file_manager_pool_.write(file, write_buffer_view, offset);
        if (result == static_cast<int>(block_size_))
        {
          statistics_.success_write.fetch_add(1, std::memory_order_relaxed);
        }
        else
        {
          statistics_.error_write.fetch_add(1, std::memory_order_relaxed);
        }
      }

      offset = 0;
      if (test_type_ == TestType::SingleFile)
      {
        if (is_iopoll || is_direct_)
        {
          offset = (dist(gen) / kPageSize) * kPageSize;
        }
        else
        {
          offset = dist(gen);
        }
      }

      if (operation_type_ == OperationType::Read || operation_type_ == OperationType::ReadWrite)
      {
        int result = file_manager_pool_.read(file, read_buffer_view, offset);
        if (result == static_cast<int>(block_size_))
        {
          statistics_.success_read.fetch_add(1, std::memory_order_relaxed);
        }
        else
        {
          statistics_.error_read.fetch_add(1, std::memory_order_relaxed);
        }
      }
    }

    if (test_type_ == TestType::FileToCoro)
    {
      std::remove(path.c_str());
    }
  }

private:
  const TestType test_type_;

  const OperationType operation_type_;

  std::atomic<bool> is_strop_{false};

  const std::string file_name = "test_file_manager";

  Logger_var logger_;

  const std::string directory_path_;

  const std::size_t number_coroutines_;

  const std::size_t file_size_;

  const std::size_t block_size_;

  const bool is_direct_;

  FileManagerConfig config_;

  UServerUtils::FileManager::FileManagerPool file_manager_pool_;

  Statistics& statistics_;

  std::vector<Task> tasks_;
};

class Application final
{
public:
  using FileManagerConfig = UServerUtils::FileManager::Config;
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using Benchmark_var = ReferenceCounting::SmartPtr<Benchmark>;

public:
  Application(
    const TestType test_type,
    const OperationType operation_type,
    const std::size_t time_interval,
    const FileManagerConfig& config,
    const std::string& directory_path,
    const std::string& file_name,
    const std::size_t number_coroutines,
    const std::size_t file_size,
    const std::size_t block_size,
    const bool is_direct)
    : test_type_(test_type),
      operation_type_(operation_type),
      time_interval_(time_interval),
      block_size_(block_size)
  {
    logger_ = new Logging::OStream::Logger(
      Logging::OStream::Config(
        std::cerr,
        Logging::Logger::INFO));

    std::ostringstream stream;
    stream << "\n\nBenchmark: \n"
           << (test_type == TestType::SingleFile ? "With single file\n" : "With file to coroutine\n");
    if (test_type == TestType::SingleFile)
    {
      stream << "File size = "
             << file_size
             << " GB\n";
    }

    stream << "Io_rings number = "
           << config.number_io_urings
           << "\nNumber coroutines = "
           << number_coroutines
           << "\nBlock size = "
           << block_size
           << " Bytes\n"
           << "Operation type = ";
    if (operation_type == OperationType::Read)
    {
      stream << "Read";
    }
    else if (operation_type == OperationType::Write)
    {
      stream << "Write";
    }
    else if (operation_type == OperationType::ReadWrite)
    {
      stream << "ReadWrite";
    }
    stream << std::endl;
    logger_->info(stream.str());

    if (test_type == TestType::SingleFile)
    {
      std::ostringstream stream;
      stream << FNS
             << "Start creation file="
             << (directory_path + file_name)
             << ". Size of file="
             << file_size
             << "GB";
      logger_->info(stream.str());
      create_file(file_size, directory_path, file_name);
      logger_->info(std::string("File is successful created"));
    }

    benchmark_ = new Benchmark(
      test_type,
      operation_type,
      logger_.in(),
      config,
      directory_path,
      file_name,
      number_coroutines,
      file_size,
      block_size,
      is_direct,
      statistics_);
  }

  void run()
  {
    logger_->info(std::string("Start benchmark"));

    Logger_var userver_logger = new Logging::OStream::Logger(
      Logging::OStream::Config(
        std::cerr,
        Logging::Logger::ERROR));

    UServerUtils::CoroPoolConfig coro_pool_config;
    coro_pool_config.initial_size = 1000;
    coro_pool_config.max_size = 10000;

    UServerUtils::EventThreadPoolConfig event_thread_pool_config;
    event_thread_pool_config.threads = 2;

    UServerUtils::TaskProcessorConfig main_task_processor_config;
    main_task_processor_config.name = "main_task_processor";
    main_task_processor_config.worker_threads =
      std::thread::hardware_concurrency();
    main_task_processor_config.thread_name = "main_tskpr";
    main_task_processor_config.should_guess_cpu_limit = false;
    main_task_processor_config.wait_queue_length_limit = 200000;

    UServerUtils::TaskProcessorContainerBuilderPtr task_processor_container_builder(
      new UServerUtils::TaskProcessorContainerBuilder(
        userver_logger.in(),
        coro_pool_config,
        event_thread_pool_config,
        main_task_processor_config));

    auto init_func =
      [benchmark = std::move(benchmark_)] (
        UServerUtils::TaskProcessorContainer& task_processor_container) {
          UServerUtils::ComponentsBuilderPtr components_builder(
            new UServerUtils::ComponentsBuilder);
          components_builder->add_user_component(
            "Benchmark",
            benchmark.in());
          return components_builder;
      };

    UServerUtils::Manager_var manager = new UServerUtils::Manager(
      std::move(task_processor_container_builder),
      std::move(init_func),
      userver_logger.in());

    manager->activate_object();

    std::atomic<bool> is_cancel(false);
    boost::scoped_thread<> thread([
      this,
      &is_cancel] () {
      try
      {
        while (!is_cancel.load(std::memory_order_relaxed))
        {
          std::this_thread::sleep_for(std::chrono::milliseconds(time_interval_ * 1000));

          const auto success_read =
            statistics_.success_read.exchange(0, std::memory_order_relaxed);
          const auto error_read =
            statistics_.error_read.exchange(0, std::memory_order_relaxed);
          const auto success_write =
            statistics_.success_write.exchange(0, std::memory_order_relaxed);
          const auto error_write =
            statistics_.error_write.exchange(0, std::memory_order_relaxed);

          logger_->info(std::string("--------------------"));
          std::ostringstream stream;

          if (operation_type_ == OperationType::Read || operation_type_ == OperationType::ReadWrite)
          {
            stream << "\n"
                   << "Success read[count/s] = "
                   << success_read / time_interval_
                   << "\n"
                   << "Error read[count/s] = "
                   << error_read / time_interval_
                   << "\n"
                   << "Read[Mb/s] = "
                   << ((success_read * block_size_) / (time_interval_ * 1048576))
                   << "\n";
          }

          if (operation_type_ == OperationType::Write || operation_type_ == OperationType::ReadWrite)
          {
            stream << "\n"
                   << "Success write[count/s] = "
                   << success_write / time_interval_
                   << "\n"
                   << "Error write[count/s] = "
                   << error_write / time_interval_
                   << "\n"
                   << "Write[Mb/s] = "
                   << ((success_write * block_size_) / (time_interval_ * 1048576))
                   << "\n";
          }

          logger_->info(stream.str());
        }
      }
      catch (...)
      {
      }
    });

    wait();

    manager->deactivate_object();
    manager->wait_object();

    is_cancel = true;
    thread.join();

    logger_->info(std::string("Benchmark is stopped"));
  }

private:
  void wait() noexcept
  {
    try
    {
      userver::utils::SignalCatcher signal_catcher{SIGINT, SIGTERM, SIGQUIT};
      signal_catcher.Catch();
    }
    catch (...)
    {
    }
  }

  void create_file(
    const std::size_t file_size,
    const std::string& directory_path,
    const std::string& file_name)
  {
    const std::string path_to_file = directory_path + file_name;
    std::remove(path_to_file.c_str());
    const std::size_t file_size_bytes = 1073741824 * file_size;

    std::ofstream ofs(path_to_file, std::ios::trunc);
    if (!ofs)
    {
      std::ostringstream stream;
      stream << FNS
             << "Can't open file="
             << path_to_file;
      throw std::runtime_error(stream.str());
    }

    const std::string data(4 * 1024 * 1024, 'a');
    const std::size_t number = file_size_bytes / data.size();
    for (std::size_t i = 1; i <= number; ++i)
    {
      if (!(ofs << data))
      {
        std::ostringstream stream;
        stream << FNS
               << "Creation file is failed";
        throw std::runtime_error(stream.str());
      }
    }
  }

private:
  const TestType test_type_;

  const OperationType operation_type_;

  Statistics statistics_;

  const std::size_t time_interval_;

  const std::size_t block_size_;

  Logger_var logger_;

  Benchmark_var benchmark_;
};

int main(int /*argc*/, char** /*argv*/)
{
  try
  {
    UServerUtils::FileManager::Config config;
    config.number_io_urings = std::thread::hardware_concurrency();
    config.io_uring_size = 10000;
    config.event_queue_max_size = 1000000;
    config.io_uring_flags = IORING_SETUP_ATTACH_WQ; // | IORING_SETUP_IOPOLL

    const OperationType operation_type = OperationType::ReadWrite;
    const TestType test_type = TestType::SingleFile;
    // https://stackoverflow.com/questions/76533640/why-o-direct-is-slower-than-normal-read
    const bool is_direct = false;
    const std::size_t time_interval = 5;
    const std::string directory_path = "/u03/test/";
    const std::string file_name = "test_file_manager";
    const std::size_t number_coroutines = 3000;
    // File size in Gb
    const std::size_t size_file = 10;
    // Read/write block in bytes
    const std::size_t block_size = 32 * 1024;

    Application application(
      test_type,
      operation_type,
      time_interval,
      config,
      directory_path,
      file_name,
      number_coroutines,
      size_file,
      block_size,
      is_direct);
    application.run();
    return EXIT_SUCCESS;
  }
  catch (const std::exception& exc)
  {
    std::cerr << "Benchmark is failed. Reason: "
              << exc.what();
  }
  catch (...)
  {
    std::cerr << "Benchmark is failed. Unknown error";
  }

  return EXIT_FAILURE;
}