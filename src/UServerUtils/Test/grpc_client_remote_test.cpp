// PROTO
#include "test_coro_client_client.cobrazz.pb.hpp"
#include "test_coro_client_service.cobrazz.pb.hpp"

// STD
#include <atomic>
#include <chrono>
#include <thread>

// THIS
#include <Generics/Uncopyable.hpp>
#include <Logger/Logger.hpp>
#include <Logger/StreamLogger.hpp>
#include <UServerUtils/Grpc/Client/PoolClientFactory.hpp>
#include <UServerUtils/ComponentsBuilder.hpp>
#include <UServerUtils/Manager.hpp>
#include <UServerUtils/Grpc/Server/ConfigCoro.hpp>

using namespace UServerUtils;
using namespace UServerUtils::Grpc::Client;

class Application final
  : protected Generics::Uncopyable
{
public:
  Application()
  {
    logger_ = new Logging::OStream::Logger(
      Logging::OStream::Config(
        std::cerr,
        Logging::Logger::ERROR));

    CoroPoolConfig coro_pool_config;
    EventThreadPoolConfig event_thread_pool_config;
    TaskProcessorConfig main_task_processor_config;
    main_task_processor_config.name = "main_task_processor";
    main_task_processor_config.worker_threads = 3;
    main_task_processor_config.thread_name = "main_tskpr";

    auto task_processor_container_builder =
      std::make_unique<TaskProcessorContainerBuilder>(
        logger_.in(),
        coro_pool_config,
        event_thread_pool_config,
        main_task_processor_config);

    auto init_func = [logger = logger_, port = port_] (
      TaskProcessorContainer& task_processor_container) {
      auto components_builder = std::make_unique<ComponentsBuilder>();

      Grpc::Server::ConfigCoro config;
      config.num_threads = 3;
      config.port = port;
      config.max_size_queue = {};

      return components_builder;
    };

    manager_ = new Manager(
      std::move(task_processor_container_builder),
      std::move(init_func),
      logger_.in());
  }

  ~Application() = default;

  int run()
  {
    manager_->activate_object();

    auto& task_processor = manager_->get_main_task_processor();

    ConfigPoolCoro config;
    config.endpoint = "127.0.0.1:" + std::to_string(port_);
    config.number_async_client = 1;
    config.number_threads = 1;

    PoolClientFactory pool_factory(
      logger_.in(),
      config);

    auto pool = pool_factory.create<
      test_coro::TestCoroService_Handler_ClientPool>(
        task_processor);
    for (int i = 1; i <= 100; ++i)
    {
      std::cout << "number: " << i << std::endl;

      auto request = std::make_unique<test_coro::Request>();
      request->set_message("hi");

      auto result = pool->write(std::move(request));
      if (result.status == Status::Ok)
      {
        std::cout << result.response->message() << std::endl;
      }
      else if (result.status == Status::Timeout)
      {
        std::cout << "timeout" << std::endl;
      }
      else if (result.status == Status::InternalError)
      {
        std::cout << "internal error" << std::endl;
      }
      else
      {
        std::cout << "unknown status" << std::endl;
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    }

    manager_->deactivate_object();
    manager_->wait_object();

    return EXIT_SUCCESS;
  }

private:
  const std::size_t port_ = 7778;

  Logging::Logger_var logger_;

  Manager_var manager_;
};

int main()
{
  try
  {
    return Application().run();
  }
  catch (const std::exception& exc)
  {
    std::cerr << "Error: " << exc.what() << std::endl;
    return EXIT_FAILURE;
  }
  catch (...)
  {
    std::cerr << "Unknown error" << std::endl;
    return EXIT_FAILURE;
  }
}