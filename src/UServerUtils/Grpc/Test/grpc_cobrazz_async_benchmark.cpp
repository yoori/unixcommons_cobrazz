// STD
#include <thread>

// BOOST
#include <boost/thread/scoped_thread.hpp>

// PROTO
#include "echo_service.cobrazz.pb.hpp"
#include "echo_client.cobrazz.pb.hpp"

// GRPC
#include <grpcpp/grpcpp.h>

// USERVER
#include <utils/signal_catcher.hpp>

// THIS
#include <Logger/Logger.hpp>
#include <Logger/StreamLogger.hpp>
#include <UServerUtils/Grpc/Core/Client/Config.hpp>
#include <UServerUtils/Grpc/Core/Common/ShutdownManager.hpp>
#include <UServerUtils/Grpc/Core/Server/Config.hpp>
#include <UServerUtils/Grpc/Core/Server/Server.hpp>

namespace Aspect
{

constexpr const char BENCHMARK[] = "BENCHMARK";

} // namespace Aspect

struct Statistics final
{
  explicit Statistics() = default;

  ~Statistics() = default;

  std::atomic<std::size_t> count_read{0};
};

class StreamStreamHandler final
  : public echo::EchoService_Handler_Handler
{
public:
  explicit StreamStreamHandler() = default;

  ~StreamStreamHandler() override = default;

  void on_request(const echo::Request& request) override
  {
    auto response = std::make_unique<echo::Reply>();
    response->set_message(request.message());
    send(std::move(response));
  }

  void initialize() override
  {
  }

  void on_reads_done() override
  {
    grpc::Status status = grpc::Status::OK;
    finish(std::move(status));
  }

  void on_finish() override
  {
  }
};

class StreamStreamClientImpl final
  : public echo::EchoService_Handler_ClientObserver
{
private:
  using WriterStatus = UServerUtils::Grpc::Core::Client::WriterStatus;
  using ShutdownManagerPtr = UServerUtils::Grpc::Core::Common::ShutdownManagerPtr;
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;

public:
  explicit StreamStreamClientImpl(
    const std::size_t count_initial,
    const std::string& message,
    const ShutdownManagerPtr& shutdown_manager,
    Logger* logger,
    Statistics& statistics)
    : count_initial_(count_initial),
      message_(message),
      shutdown_manager_(shutdown_manager),
      logger_(ReferenceCounting::add_ref(logger)),
      statistics_(statistics)
  {
  }

  ~StreamStreamClientImpl() override = default;

private:
  void on_data(
    const ClientId& /*client_id*/,
    const CompletionQueuePtr& /*completion_queue*/,
    const ChannelPtr& /*channel*/) override
  {
  }

  void on_writer(WriterPtr&& writer) override
  {
    writer_ = std::move(writer);
  }

  void on_initialize(const bool ok) override
  {
    if (ok)
    {
      for (std::size_t i = 1; i <= count_initial_; ++i)
      {
        auto request = std::make_unique<echo::Request>();
        request->set_message(message_);
        const auto status = writer_->write(std::move(request));
        if (status != WriterStatus::Ok)
        {
          Stream::Error stream;
          stream << FNS
                 << ": write is failed";
          logger_->error(stream.str(), Aspect::BENCHMARK);
        }
      }
    }
    else
    {
      Stream::Error stream;
      stream << FNS
             << ": initialization is failed";
      logger_->error(stream.str(), Aspect::BENCHMARK);
    }
  }

  void on_read(Response&& response) override
  {
    if (response.message() == message_)
    {
      statistics_.count_read.fetch_add(1, std::memory_order_relaxed);
    }
    else
    {
      Stream::Error stream;
      stream << FNS
             << ": Not correct string";
      logger_->error(stream.str(), Aspect::BENCHMARK);
    }

    auto request = std::make_unique<echo::Request>();
    request->set_message(message_);
    const auto status = writer_->write(std::move(request));
    if (status != WriterStatus::Ok)
    {
      Stream::Error stream;
      stream << FNS
             << ": write is failed";
      logger_->error(stream.str(), Aspect::BENCHMARK);
    }
  }

  void on_finish(grpc::Status&& /*status*/) override
  {
    shutdown_manager_->shutdown();
  }

private:
  const std::size_t count_initial_;

  const std::string message_;

  const ShutdownManagerPtr shutdown_manager_;

  const Logger_var logger_;

  Statistics& statistics_;

  WriterPtr writer_;
};

class PoolClient final
  : protected Generics::Uncopyable
{
private:
  using Factory = echo::EchoService_Handler_Factory;
  using FactoryPtr = std::unique_ptr<Factory>;
  using ShutdownManager =
    UServerUtils::Grpc::Core::Common::ShutdownManager;
  using ShutdownManagerPtr =
    UServerUtils::Grpc::Core::Common::ShutdownManagerPtr;
  using Impl = StreamStreamClientImpl;
  using ImplPtr = std::unique_ptr<Impl>;
  using Impls = std::vector<std::pair<ImplPtr, ShutdownManagerPtr>>;
  using Config = UServerUtils::Grpc::Core::Client::Config;
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;

public:
  explicit PoolClient(
    const std::size_t count_initial,
    const std::string& message,
    const Config& config,
    const std::size_t number_async_client,
    Logger* logger,
    Statistics& statistics)
    : logger_(ReferenceCounting::add_ref(logger)),
      factory_(std::make_unique<Factory>(config, logger))
  {
    impls_.reserve(number_async_client);
    for (std::size_t i = 1; i <= number_async_client; ++i)
    {
      ShutdownManagerPtr shutdown_manager(new ShutdownManager);
      impls_.emplace_back(
        std::make_unique<Impl>(
          count_initial,
          message,
          shutdown_manager,
          logger,
          statistics),
        shutdown_manager);
      factory_->create(*impls_.back().first);
    }
  }

  ~PoolClient()
  {
    for (auto& impl : impls_)
    {
      try
      {
        impl.second->wait();
      }
      catch (...)
      {
      }
    }
  }

private:
  Logger_var logger_;

  Impls impls_;

  FactoryPtr factory_;
};

class Application : private Generics::Uncopyable
{
private:
  using Server = UServerUtils::Grpc::Core::Server::Server;
  using Server_var = UServerUtils::Grpc::Core::Server::Server_var;
  using Logger_var = Logging::Logger_var;

public:
  Application(
    const std::size_t port,
    const std::size_t time_interval,
    const std::string message,
    const std::size_t count_initial,
    const std::size_t number_server_thread,
    const std::size_t number_client_thread,
    const std::size_t number_channels,
    const std::size_t number_async_client)
    : port_(port),
      time_interval_(time_interval),
      message_(message),
      count_initial_(count_initial),
      number_server_thread_(number_server_thread),
      number_client_thread_(number_client_thread),
      number_channels_(number_channels),
      number_async_client_(number_async_client)
  {
  }

  void run()
  {
    Logger_var logger(
      new Logging::OStream::Logger(
        Logging::OStream::Config(
          std::cerr,
          Logging::Logger::INFO)));

    Statistics statistics;

    UServerUtils::Grpc::Core::Server::Config server_config;
    server_config.num_threads = number_server_thread_;
    server_config.port = port_;

    Server_var server(
      new Server(
        server_config,
        logger.in()));
    server->register_handler<StreamStreamHandler>();
    server->activate_object();

    std::stringstream stream;
    stream << "Server is started on port="
           << port_;
    logger->info(stream.str(), Aspect::BENCHMARK);

    UServerUtils::Grpc::Core::Client::Config client_config;
    client_config.number_threads = number_client_thread_;
    client_config.number_channels = number_channels_;
    client_config.endpoint = "0.0.0.0:" + std::to_string(port_);

    auto pool_client = std::make_unique<PoolClient>(
      count_initial_,
      message_,
      client_config,
      number_async_client_,
      logger.in(),
      statistics);

    std::atomic<bool> is_cancel(false);
    boost::scoped_thread<> thread([
      logger,
      &statistics,
      &is_cancel,
      time_interval = time_interval_] () {
      try
      {
        while (!is_cancel.load(std::memory_order_relaxed))
        {
          std::this_thread::sleep_for(std::chrono::milliseconds(time_interval * 1000));

          const std::size_t count_read = statistics.count_read.exchange(
            0,
            std::memory_order_relaxed);
          std::stringstream stream;
          stream << "Success read[rq/s] = "
                 << (count_read / time_interval);
          logger->info(stream.str(), Aspect::BENCHMARK);
        }
      }
      catch (const eh::Exception& exc)
      {
      }
    });

    wait();
    logger->info(
      std::string("Stopping benchmark. Please wait..."),
      Aspect::BENCHMARK);

    is_cancel.store(true);
    server->deactivate_object();
    server->wait_object();

    pool_client.reset();

    logger->info(
      std::string("Benchmark is stopped"),
      Aspect::BENCHMARK);
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

  const std::size_t time_interval_;

  const std::string message_;

  const std::size_t count_initial_;

  const std::size_t number_server_thread_;

  const std::size_t number_client_thread_;

  const std::size_t number_channels_;

  const std::size_t number_async_client_;
};

int main(int /*argc*/, char** /*argv*/)
{
  try
  {
    const std::size_t port = 8888;
    const std::size_t time_interval = 5;
    const std::string message = "hello";
    const std::size_t count_initial = 400;
    const std::size_t number_server_thread = std::thread::hardware_concurrency();
    const std::size_t number_client_thread = std::thread::hardware_concurrency();
    const std::size_t number_channels = number_client_thread;
    const std::size_t number_async_client = number_channels;

    Application(
      port,
      time_interval,
      message,
      count_initial,
      number_server_thread,
      number_client_thread,
      number_channels,
      number_async_client).run();

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