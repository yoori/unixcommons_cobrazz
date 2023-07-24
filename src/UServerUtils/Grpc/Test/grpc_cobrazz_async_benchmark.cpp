// PROTO
#include "echo_service.cobrazz.pb.hpp"

// GRPC
#include <grpcpp/grpcpp.h>

// USERVER
#include <utils/signal_catcher.hpp>

// THIS
#include <Logger/Logger.hpp>
#include <Logger/StreamLogger.hpp>
#include <UServerUtils/Grpc/Core/Server/Config.hpp>
#include <UServerUtils/Grpc/Core/Server/Server.hpp>

class StreamStreamHandler
  : public echo::EchoService_Handler_Handler
{
public:
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


int main(int /*argc*/, char** /*argv*/)
{
  try
  {
    Logging::Logger_var logger(
      new Logging::OStream::Logger(
        Logging::OStream::Config(
          std::cerr,
          Logging::Logger::INFO)));

    UServerUtils::Grpc::Core::Server::Config config;
    config.num_threads = 24;
    config.port = 7778;

    UServerUtils::Grpc::Core::Server::Server_var server(
      new UServerUtils::Grpc::Core::Server::Server(
        config,
        logger.in()));
    server->register_handler<StreamStreamHandler>();

    server->activate_object();

    std::cout << "Server is started on port="
              << config.port
              << std::endl;

    userver::utils::SignalCatcher signal_catcher{SIGINT, SIGTERM, SIGQUIT};
    signal_catcher.Catch();

    server->deactivate_object();
    server->wait_object();

    std::cout << "Server is finished" << std::endl;
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
