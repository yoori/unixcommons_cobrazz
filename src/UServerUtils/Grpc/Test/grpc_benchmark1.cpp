// STD
#include <atomic>
#include <chrono>
#include <deque>
#include <iostream>
#include <thread>

// BOOST
#include <boost/thread/scoped_thread.hpp>

// PROTO
#include "test2_service.usrv.pb.hpp"
#include "test2_client.usrv.pb.hpp"

// USERVER
#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task.hpp>
#include <utils/signal_catcher.hpp>

// THIS
#include <eh/Exception.hpp>
#include <Logger/Logger.hpp>
#include <Logger/StreamLogger.hpp>
#include "../ComponentsBuilder.hpp"
#include "../Manager.hpp"

using namespace UServerUtils::Grpc;
using TaskProcessor = TaskProcessorContainer::TaskProcessor;

struct Statistics final
{
  explicit Statistics() = default;

  ~Statistics() = default;

  std::atomic<std::size_t> success_write{0};
  std::atomic<std::size_t> error_write{0};
  std::atomic<std::size_t> success_read{0};
  std::atomic<std::size_t> error_read{0};
};

class Service final
  : public Test2::TestStreamServiceBase,
    public ReferenceCounting::AtomicImpl
{
public:
  explicit Service() = default;

  ~Service() override = default;

  void chat(chatCall& call) override
  {
    Test2::Request request;
    Test2::Response response;
    while (call.Read(request))
    {
      response.set_allocated_response(request.release_request());
      call.Write(response);
      userver::engine::Yield();
    }
    call.Finish();
  }
};

class Client final
  : public Generics::ActiveObject,
    public ReferenceCounting::AtomicImpl
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  explicit Client(
    const std::string& message,
    const std::size_t number_initial_message,
    const GrpcClientFactory_var& factory,
    const std::string& endpoint,
    TaskProcessor& task_processor,
    Statistics& statistics)
    : message_(message),
      number_initial_message_(number_initial_message),
      client_(factory->make_client<Test2::TestStreamServiceClient>(
        "Client",
        endpoint)),
      task_processor_(task_processor),
      statistics_(statistics)
  {
    if (number_initial_message == 0)
    {
      Stream::Error stream;
      stream << FNS
             << " : number_initial_message must be more then 0";
      throw Exception(stream);
    }
  }

  ~Client() override = default;

  void activate_object() override
  {
    const bool is_task_processor_thread =
      userver::engine::current_task::IsTaskProcessorThread();
    if (!is_task_processor_thread)
    {
      Stream::Error stream;
      stream << FNS
             << ": Not task processor thread";
      throw Exception(stream);
    }

    task_ = userver::engine::AsyncNoSpan(
      task_processor_,
      [this] () {
      this->run();
    });
  }

  void deactivate_object() override
  {
    is_stop_.store(true, std::memory_order_relaxed);
    try
    {
      if (task_.IsValid())
      {
        task_.Get();
      }
    }
    catch (const eh::Exception& exc)
    {
      std::cerr << FNS
                << " : "
                << exc.what();
    }
  }

  void wait_object() override
  {
  }

  bool active() override
  {
    using State = userver::engine::Task::State;
    const auto state = task_.GetState();
    return state != State::kInvalid
        && state != State::kCompleted;
  }

private:
  void run()
  {
    while (!is_stop_.load(std::memory_order_relaxed))
    {
      userver::engine::Yield();
      try
      {
        auto context = std::make_unique<grpc::ClientContext>();
        auto call = client_->chat(std::move(context));

        Test2::Request request;
        request.set_request(message_);
        for (std::size_t i = 1; i <= number_initial_message_; ++i)
        {
          if (!call.Write(request))
          {
            Stream::Error stream;
            stream << FNS
                   << " : Write is failed";
            throw Exception(stream);
          }
          statistics_.success_write.fetch_add(1, std::memory_order_relaxed);
          userver::engine::Yield();
        }

        Test2::Response response;
        while (!is_stop_.load(std::memory_order_relaxed))
        {
          if (call.Read(response))
          {
            statistics_.success_read.fetch_add(1, std::memory_order_relaxed);
          } else
          {
            statistics_.error_read.fetch_add(1, std::memory_order_relaxed);
          }

          userver::engine::Yield();

          if (call.Write(request))
          {
            statistics_.success_write.fetch_add(1, std::memory_order_relaxed);
          } else
          {
            statistics_.error_write.fetch_add(1, std::memory_order_relaxed);
          }
        }

        if (!call.WritesDone())
        {
          Stream::Error stream;
          stream << FNS
                 << " : WritesDone is failed";
          throw Exception(stream);
        }
      }
      catch (const eh::Exception &exc)
      {
        std::cerr << "Error: "
                  << exc.what()
                  << std::endl;
      }
    }
  }

private:
  const std::string message_;

  const std::size_t number_initial_message_;

  std::unique_ptr<Test2::TestStreamServiceClient> client_;

  std::atomic<bool> is_stop_{false};

  TaskProcessor& task_processor_;

  Statistics& statistics_;

  userver::engine::TaskWithResult<void> task_;
};

class Benchmark final
  : public Component,
    public ReferenceCounting::AtomicImpl
{
public:
  Benchmark(
    const std::size_t number_client,
    const std::string& message,
    const std::size_t number_initial_message,
    const GrpcClientFactory_var& factory,
    const std::string& endpoint,
    TaskProcessor& task_processor,
    Statistics& statistics)
  {
    for (std::size_t i = 1; i <= number_client; ++i)
    {
      ReferenceCounting::SmartPtr<Client> client(
        new Client(
          message,
          number_initial_message,
          factory,
          endpoint,
          task_processor,
          statistics));
      add_child_object(client.in());
    }
  }

  ~Benchmark() override = default;
};

class Application : Generics::Uncopyable
{
public:
  Application(
    const std::size_t port,
    const std::size_t number_client,
    const std::string& message,
    const std::size_t number_initial_message,
    const std::size_t time_interval,
    const std::size_t  number_server_thread,
    const std::size_t  number_client_thread,
    const std::size_t number_channel_thread,
    const std::size_t number_channel)
    : port_(port),
      number_client_(number_client),
      message_(message),
      number_initial_message_(number_initial_message),
      time_interval_(time_interval),
      number_server_thread_(number_server_thread),
      number_client_thread_(number_client_thread),
      number_channel_thread_(number_channel_thread),
      number_channel_(number_channel)
  {
  }

  int run()
  {
    std::cout << "Start benchmark with parameters : "
              << "\n\n"
              << "number_client = "
              << number_client_
              << "\n"
              << "message = "
              << message_
              << "\n"
              << "number_initial_message = "
              << number_initial_message_
              << "\n"
              << "time_interval = "
              << time_interval_
              << "\n"
              << "number server threads = "
              << number_server_thread_
              << "\n"
              << "number client threads = "
              << number_client_thread_
              << "\n"
              << "number chennel threads = "
              << number_channel_thread_
              << "\n"
              << "number channels = "
              << number_channel_
              << "\n"
              << std::endl;

    Logging::Logger_var logger(
      new Logging::OStream::Logger(
        Logging::OStream::Config(
          std::cerr,
          Logging::Logger::ERROR)));

    CoroPoolConfig coro_pool_config;
    coro_pool_config.initial_size = 1000;
    coro_pool_config.max_size = 10000;

    EventThreadPoolConfig event_thread_pool_config;
    event_thread_pool_config.threads = 5;

    TaskProcessorConfig main_task_processor_config;
    main_task_processor_config.name = "main_task_processor";
    main_task_processor_config.worker_threads = number_server_thread_;
    main_task_processor_config.thread_name = "main_tskpr";
    main_task_processor_config.should_guess_cpu_limit = false;
    main_task_processor_config.wait_queue_length_limit = 100000;

    TaskProcessorContainerBuilderPtr task_processor_container_builder(
        new TaskProcessorContainerBuilder(
          logger.in(),
          coro_pool_config,
          event_thread_pool_config,
          main_task_processor_config));

    const std::string name_channel_task_processor = "channel_task_processor";
    TaskProcessorConfig channel_task_processor_config;
    channel_task_processor_config.name = name_channel_task_processor;
    channel_task_processor_config.worker_threads = number_channel_thread_;
    channel_task_processor_config.thread_name = "channel_tskpr";
    channel_task_processor_config.wait_queue_length_limit = 100000;

    task_processor_container_builder->add_task_processor(
      channel_task_processor_config);

    const std::string name_client_task_processor = "client_task_processor";
    TaskProcessorConfig client_task_processor_config;
    client_task_processor_config.name = name_client_task_processor;
    client_task_processor_config.worker_threads = number_client_thread_;
    client_task_processor_config.thread_name = "client_tskpr";
    client_task_processor_config.wait_queue_length_limit = 100000;

    task_processor_container_builder->add_task_processor(
      client_task_processor_config);

    Statistics statistics;

    auto inititialize_func = [
      this,
      &name_channel_task_processor,
      &name_client_task_processor,
      &statistics,
      logger] (
      TaskProcessorContainer& task_processor_container) {
      auto& main_task_processor =
        task_processor_container.get_main_task_processor();

      ComponentsBuilderPtr components_builder(new ComponentsBuilder);
      auto& statistic_storage = components_builder->get_statistics_storage();

      GrpcServerConfig config_server;
      config_server.port = port_;
      auto registrator_dynamic_settings =
        components_builder->registrator_dynamic_settings();
      GrpcServerBuilderPtr server_builder =
        std::make_unique<GrpcServerBuilder>(
          logger.in(),
          std::move(config_server),
          statistic_storage,
          registrator_dynamic_settings);
      GrpcServiceBase_var service(new Service);
      server_builder->add_grpc_service(
        main_task_processor,
        service.in());
      components_builder->add_grpc_server(std::move(server_builder));

      GrpcClientFactoryConfig client_factory_config;
      client_factory_config.channel_count = number_channel_;

      auto& channel_task_processor =
        task_processor_container.get_task_processor(
          name_channel_task_processor);
      auto client_factory = components_builder->add_grpc_client_factory(
        std::move(client_factory_config),
        channel_task_processor,
        nullptr);

      auto& client_task_processor =
        task_processor_container.get_task_processor(
          name_client_task_processor);
      ReferenceCounting::SmartPtr<Benchmark> benchmark(
        new Benchmark(
          number_client_,
          message_,
          number_initial_message_,
          client_factory,
          "127.0.0.1:" + std::to_string(port_),
          client_task_processor,
          statistics));

      components_builder->add_user_component("Benchmark", benchmark.in());

      return components_builder;
    };

    Manager_var manager(
      new Manager(
        std::move(task_processor_container_builder),
        std::move(inititialize_func),
        logger.in()));

    manager->activate_object();

    std::atomic<bool> is_cancel(false);
    boost::scoped_thread<> thread([
      &statistics,
      &is_cancel,
      time_interval = time_interval_] () {
      try
      {
        while (!is_cancel.load(std::memory_order_relaxed))
        {
          std::this_thread::sleep_for(std::chrono::milliseconds(time_interval * 1000));

          const std::size_t success_write =
            statistics.success_write.exchange(0, std::memory_order_relaxed);
          const std::size_t error_write =
            statistics.error_write.exchange(0, std::memory_order_relaxed);
          const std::size_t success_read =
            statistics.success_read.exchange(0, std::memory_order_relaxed);
          const std::size_t error_read =
            statistics.error_read.exchange(0, std::memory_order_relaxed);
          std::cout << "---------------" << std::endl;
          std::cout << "Success read[rq/s] = "
                    << success_read / time_interval
                    << "\n"
                    << "Error read[rq/s] = "
                    << error_read / time_interval
                    << "\n"
                    << "Success write[rq/s] = "
                    << success_write / time_interval
                    << "\n"
                    << "Error write[rq/s] = "
                    << error_write / time_interval
                    << std::endl;
        }
      }
      catch (const eh::Exception& exc)
      {
      }
    });

    wait();
    std::cout << "Stopping benchmark. Please wait..." << std::endl;

    is_cancel.store(true);
    manager->deactivate_object();
    manager->wait_object();

    return EXIT_SUCCESS;
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
  const std::size_t port_;

  const std::size_t number_client_;

  const std::string message_;

  const std::size_t number_initial_message_;

  const std::size_t time_interval_;

  const std::size_t  number_server_thread_;

  const std::size_t  number_client_thread_;

  const std::size_t number_channel_thread_;

  const std::size_t number_channel_;
};

int main(int /*argc*/, char** /*argv*/)
{
  try
  {
    const std::size_t port = 8888;
    const std::size_t number_client = 200;
    const std::string message = "hello";
    const std::size_t number_initial_message = 5;
    const std::size_t time_interval = 5;
    const std::size_t number_server_thread = 5;
    const std::size_t number_client_thread = 10;
    const std::size_t number_channel_thread = 5;
    const std::size_t number_channel = 2;

    return Application(
      port,
      number_client,
      message,
      number_initial_message,
      time_interval,
      number_server_thread,
      number_client_thread,
      number_channel_thread,
      number_channel).run();
  }
  catch (const eh::Exception& exc)
  {
    std::cerr << "Fatal error : "
              << exc.what()
              << std::endl;
  }
}