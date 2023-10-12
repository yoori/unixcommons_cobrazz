// STD
#include <atomic>
#include <iostream>

// USERVER
#include <userver/engine/async.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/utils/async.hpp>
#include <engine/task/task_processor.hpp>

// THIS
#include <Logger/Logger.hpp>
#include <Logger/StreamLogger.hpp>
#include <UServerUtils/Grpc/FileManager/Config.hpp>
#include <UServerUtils/Grpc/FileManager/FileManager.hpp>
#include <UServerUtils/Grpc/Component.hpp>
#include <UServerUtils/Grpc/ComponentsBuilder.hpp>
#include <UServerUtils/Grpc/ComponentsBuilder.hpp>
#include <UServerUtils/Grpc/Manager.hpp>
#include <UServerUtils/Grpc/Utils.hpp>

using namespace UServerUtils::Grpc;
using TaskProcessor = TaskProcessorContainer::TaskProcessor;

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
  : public Component,
    public ReferenceCounting::AtomicImpl
{
public:
  using FileManagerConfig = FileManager::Config;
  using Task = userver::engine::TaskWithResult<void>;
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;

public:
  Benchmark(
    Logger* logger,
    const FileManagerConfig& config,
    const std::string& directory_path,
    const std::size_t number_coroutines,
    const std::string& write_data,
    Statistics& statistics)
  : logger_(ReferenceCounting::add_ref(logger)),
    directory_path_(directory_path),
    number_coroutines_(number_coroutines),
    write_data_(write_data),
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
      tasks_.emplace_back(userver::utils::Async(
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
    const std::string path =
      directory_path_ + file_name + std::to_string(number);
    std::remove(path.c_str());

    FileManager::File file(path, O_CREAT | O_RDWR);
    if (!file.is_valid())
    {
      std::ostringstream stream;
      stream << FNS
             << "file="
             << path
             << " not valid, reason="
             << file.error_message();
      logger_->error(stream.str());
    }

    FileManager::FileManager file_manager(config_, logger_.in());

    std::string read_buffer;
    read_buffer.resize(write_data_.size());

    while (!is_strop_)
    {
      int result = file_manager.write(file, write_data_, 0);
      if (result == static_cast<int>(write_data_.size()))
      {
        statistics_.success_write.fetch_add(1, std::memory_order_relaxed);
      }
      else
      {
        statistics_.error_write.fetch_add(1, std::memory_order_relaxed);
      }

      result = file_manager.read(file, read_buffer, 0);
      if (result == static_cast<int>(write_data_.size()))
      {
        statistics_.success_read.fetch_add(1, std::memory_order_relaxed);
      }
      else
      {
        statistics_.error_read.fetch_add(1, std::memory_order_relaxed);
      }
    }

    std::remove(path.c_str());
  }

private:
  std::atomic<bool> is_strop_{false};

  const std::string file_name = "test_file_manager";

  Logger_var logger_;

  const FileManagerConfig config_;

  const std::string directory_path_;

  const std::size_t number_coroutines_;

  const std::string write_data_;

  Statistics& statistics_;

  std::vector<Task> tasks_;
};

class Application final
{
public:
  using FileManagerConfig = FileManager::Config;
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using Benchmark_var = ReferenceCounting::SmartPtr<Benchmark>;

public:
  Application(
    const std::size_t time_interval,
    const FileManagerConfig& config,
    const std::string& directory_path,
    const std::size_t number_coroutines,
    const std::string& write_data)
    : time_interval_(time_interval)
  {
    logger_ = new Logging::OStream::Logger(
      Logging::OStream::Config(
        std::cerr,
        Logging::Logger::INFO));

    benchmark_ = new Benchmark(
      logger_.in(),
      config,
      directory_path,
      number_coroutines,
      write_data,
      statistics_);
  }

  void run()
  {
    logger_->info(std::string("Start benchmark"));

    CoroPoolConfig coro_pool_config;
    coro_pool_config.initial_size = 1000;
    coro_pool_config.max_size = 10000;

    EventThreadPoolConfig event_thread_pool_config;
    event_thread_pool_config.threads = 2;

    TaskProcessorConfig main_task_processor_config;
    main_task_processor_config.name = "main_task_processor";
    main_task_processor_config.worker_threads = std::thread::hardware_concurrency();
    main_task_processor_config.thread_name = "main_tskpr";
    main_task_processor_config.should_guess_cpu_limit = false;
    main_task_processor_config.wait_queue_length_limit = 100000;

    TaskProcessorContainerBuilderPtr task_processor_container_builder(
      new TaskProcessorContainerBuilder(
         logger_.in(),
          coro_pool_config,
          event_thread_pool_config,
          main_task_processor_config));

    auto init_func =
      [benchmark = std::move(benchmark_)] (TaskProcessorContainer& task_processor_container) {
        ComponentsBuilderPtr components_builder(new ComponentsBuilder);
        components_builder->add_user_component("Benchmark", benchmark.in());
        return components_builder;
    };

    Manager_var manager(
      new Manager(
        std::move(task_processor_container_builder),
        std::move(init_func),
        logger_.in()));

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
          stream << "\n"
                 << "Success read[count/s] = "
                 << success_read / time_interval_
                 << "\n"
                 << "Error read[count/s] = "
                 << error_read / time_interval_
                 << "\n"
                 << "Success write[count/s] = "
                 << success_write / time_interval_
                 << "\n"
                 << "Error write[count/s] = "
                 << error_write / time_interval_
                 << "\n";
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

private:
  Statistics statistics_;

  const std::size_t time_interval_;

  Logger_var logger_;

  Benchmark_var benchmark_;
};

int main(int /*argc*/, char** /*argv*/)
{
  try
  {
    FileManager::Config config;
    config.number_io_urings = 5;
    config.io_uring_size = 10000;
    config.io_uring_flags = IORING_SETUP_ATTACH_WQ;

    const std::size_t time_interval = 2;
    const std::string directory_path = "/tmp/";
    const std::size_t number_coroutines = 300;
    const std::string write_data(100, 'a');

    Application application(
      time_interval,
      config,
      directory_path,
      number_coroutines,
      write_data);
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