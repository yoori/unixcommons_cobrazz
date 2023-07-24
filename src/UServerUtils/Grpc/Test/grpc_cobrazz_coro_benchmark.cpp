// PROTO
#include "echo_service.cobrazz.pb.hpp"

// USERVER
#include <utils/signal_catcher.hpp>

// THIS
#include <Logger/Logger.hpp>
#include <Logger/StreamLogger.hpp>
#include <UServerUtils/Grpc/CobrazzServerBuilder.hpp>
#include <UServerUtils/Grpc/ComponentsBuilder.hpp>
#include <UServerUtils/Grpc/Manager.hpp>
#include <UServerUtils/Grpc/Core/Server/ConfigCoro.hpp>

using namespace UServerUtils::Grpc;

class StreamStreamService final
  : public echo::EchoService_Handler_Service,
    public ReferenceCounting::AtomicImpl
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;

public:
  StreamStreamService(Logger* logger)
    : logger_(ReferenceCounting::add_ref(logger))
  {
  }

  ~StreamStreamService() override = default;

  void handle(const Reader& reader) override
  {
    while (true)
    {
      const auto data = reader.read();
      const auto status = data.status;
      if (status == ReadStatus::Read)
      {
        auto& request = data.request;
        if (!request)
        {
          Stream::Error stream;
          stream << FNS
                 << ": request is null";
          logger_->error(stream.str());
          continue;
        }

        auto &writer = data.writer;
        if (!writer)
        {
          Stream::Error stream;
          stream << FNS
                 << ": writer is null";
          logger_->error(stream.str());
          continue;
        }

        auto& message = request->message();
        auto response = std::make_unique<Response>();
        response->set_message(std::move(message));
        auto writer_status = writer->write(std::move(response));
        if (writer_status != WriterStatus::Ok)
        {
          Stream::Error stream;
          stream << FNS
                 << ": write is failed";
          logger_->error(stream.str());
        }
      }
      else if (status == ReadStatus::ReadsDone)
      {
        auto& writer = data.writer;
        if (!writer)
        {
          Stream::Error stream;
          stream << FNS
                 << ": writer is null";
          logger_->error(stream.str());
          continue;
        }

        grpc::Status status = grpc::Status::OK;
        const auto writer_status = writer->finish(std::move(status));
        if (writer_status != WriterStatus::Ok)
        {
          Stream::Error stream;
          stream << FNS
                 << ": finish is failed";
          logger_->error(stream.str());
        }
      }
      else if (status == ReadStatus::Finish)
      {
        break;
      }
    }
  }

public:
  Logger_var logger_;
};

int main(int /*argc*/, char** /*argv*/)
{
  try
  {
    const std::size_t port = 7777;

    Logging::Logger_var logger(
      new Logging::OStream::Logger(
        Logging::OStream::Config(
          std::cerr,
          Logging::Logger::INFO)));

    CoroPoolConfig coro_pool_config;
    coro_pool_config.initial_size = 1000;
    coro_pool_config.max_size = 20000;

    EventThreadPoolConfig event_thread_pool_config;
    event_thread_pool_config.threads = 2;

    TaskProcessorConfig main_task_processor_config;
    main_task_processor_config.name = "main_task_processor";
    main_task_processor_config.worker_threads = 24;
    main_task_processor_config.thread_name = "main_tskpr";

    auto task_processor_container_builder =
      std::make_unique<TaskProcessorContainerBuilder>(
        logger,
        coro_pool_config,
        event_thread_pool_config,
        main_task_processor_config);

    auto init_func = [logger, port] (
      TaskProcessorContainer& task_processor_container) {
      auto& main_task_processor =
        task_processor_container.get_main_task_processor();

      auto components_builder =
        std::make_unique<ComponentsBuilder>();

      Core::Server::ConfigCoro config;
      config.num_threads = 24;
      config.port = port;
      config.max_size_queue = {};

      auto grpc_builder =
        std::make_unique<GrpcCobrazzServerBuilder>(
          config,
          logger.in());

      echo::EchoService_Handler_Service_var service(
        new StreamStreamService(logger.in()));
      grpc_builder->add_service(
        service.in(),
        main_task_processor);

      components_builder->add_grpc_cobrazz_server(
        std::move(grpc_builder));

      return components_builder;
    };

    Manager_var manager(
      new Manager(
        std::move(task_processor_container_builder),
        std::move(init_func),
        logger));
    manager->activate_object();

    userver::utils::SignalCatcher signal_catcher{SIGINT, SIGTERM, SIGQUIT};
    signal_catcher.Catch();

    manager->deactivate_object();
    manager->wait_object();
  }
  catch (const std::exception& exc)
  {
    std::cerr << "Server is failed. Reason: "
              << exc.what();
  }
  catch (...)
  {
    std::cerr << "Server is failed. Unknown error";
  }
}
