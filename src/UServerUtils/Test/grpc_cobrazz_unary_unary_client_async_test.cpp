// GTEST
#include <gtest/gtest.h>

// PROTO
#include "test_client.cobrazz.pb.hpp"
#include "test_service.cobrazz.pb.hpp"

// GRPC
#include <grpcpp/grpcpp.h>

// STD
#include <atomic>

// THIS
#include <Logger/Logger.hpp>
#include <Logger/StreamLogger.hpp>
#include <UServerUtils/Grpc/Common/ShutdownManager.hpp>
#include <UServerUtils/Grpc/Client/Config.hpp>
#include <UServerUtils/Grpc/Server/Config.hpp>
#include <UServerUtils/Grpc/Server/Server.hpp>

namespace
{

namespace Client = UServerUtils::Grpc::Client;
namespace Common = UServerUtils::Grpc::Common;

const std::string kRequestOk = "Ok";
const std::size_t kNumberRequest = 100;
std::atomic<int> kCounterClientUnaryUnary{0};

class UnaryUnaryHandler final
  : public test::TestService_HandlerUnaryUnary_Handler
{
public:
  void on_request(const test::Request& request) override
  {
    auto response = std::make_unique<test::Reply>();
    response->set_message(request.message());
    send(std::move(response));
  }

  void initialize() override
  {
  }

  void on_reads_done() override
  {
  }

  void on_finish() override
  {
  }
};

class UnaryUnaryClientImpl final :
  public test::TestService_HandlerUnaryUnary_ClientObserver
{
public:
  UnaryUnaryClientImpl(
    const Common::ShutdownManagerPtr& shutdown_manager)
    : shutdown_manager_(shutdown_manager)
  {
  }

  ~UnaryUnaryClientImpl() override = default;

private:
  void on_finish(
    grpc::Status&& status,
    test::Reply&& response) override
  {
    kCounterClientUnaryUnary.fetch_add(1);
    EXPECT_EQ(status.error_code(), grpc::StatusCode::OK);
    EXPECT_EQ(response.message(), kRequestOk);

    const auto count = counter_.fetch_add(1, std::memory_order_acq_rel);
    if (count == kNumberRequest - 1)
    {
      shutdown_manager_->shutdown();
    }
  }

private:
  Common::ShutdownManagerPtr shutdown_manager_;

  std::atomic<std::size_t> counter_{0};
};

class UnaryUnaryClient final
{
private:
  using Factory = test::TestService_HandlerUnaryUnary_Factory;
  using Config = Client::Config;
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using Impl = UnaryUnaryClientImpl;

public:
  UnaryUnaryClient(
    const Common::ShutdownManagerPtr& shutdown_manager,
    const Config& config,
    Logger* logger)
    : shutdown_manager_(shutdown_manager),
      factory_(std::make_unique<Factory>(config, logger))
  {
  }

  ~UnaryUnaryClient()
  {
    EXPECT_EQ(factory_->size(), 0);
  }

  void start()
  {
    auto impl = std::make_shared<Impl>(shutdown_manager_);
    for (std::size_t i = 1; i <= kNumberRequest; ++i)
    {
      auto request = std::make_unique<test::Request>();
      request->set_message(kRequestOk);
      factory_->create(
        impl,
        std::move(request));
    }
  }

private:
  const Common::ShutdownManagerPtr shutdown_manager_;

  const std::unique_ptr<Factory> factory_;
};

class GrpcFixtureUnaryUnary_Client
  : public testing::Test
{
public:
  void SetUp() override
  {
    logger_ = new Logging::OStream::Logger(
      Logging::OStream::Config(
        std::cerr,
        Logging::Logger::CRITICAL));

    UServerUtils::Grpc::Server::Config config;
    config.num_threads = 3;
    config.port = port_;

    server_ = new UServerUtils::Grpc::Server::Server(
      config,
      logger_.in());
    server_->register_handler<UnaryUnaryHandler>();
  }

  void TearDown() override
  {
  }

  std::size_t port_ = 7778;

  Logging::Logger_var logger_;

  UServerUtils::Grpc::Server::Server_var server_;
};

} // namespace

TEST_F(GrpcFixtureUnaryUnary_Client, UnaryUnary_Client)
{
  server_->activate_object();

  Common::ShutdownManagerPtr shutdown_manager =
    std::make_shared<Common::ShutdownManager>();
  Client::Config client_config;
  client_config.endpoint =
    "127.0.0.1:" + std::to_string(port_);
  UnaryUnaryClient client(
    shutdown_manager,
    client_config,
    logger_.in());
  client.start();
  shutdown_manager->wait();

  EXPECT_EQ(kNumberRequest, kCounterClientUnaryUnary.exchange(0));

  server_->deactivate_object();
  server_->wait_object();
}

namespace
{

class UnaryUnaryClient_ServerNotExistImpl final:
  public test::TestService_HandlerUnaryUnary_ClientObserver
{
public:
  UnaryUnaryClient_ServerNotExistImpl(
    const Common::ShutdownManagerPtr& shutdown_manager)
    : shutdown_manager_(shutdown_manager)
  {
  }

  ~UnaryUnaryClient_ServerNotExistImpl() override = default;

private:
  void on_finish(
    grpc::Status&& status,
    test::Reply&& response) override
  {
    EXPECT_TRUE(true);
    EXPECT_EQ(status.error_code(), grpc::StatusCode::UNAVAILABLE);
    shutdown_manager_->shutdown();
  }

private:
  const Common::ShutdownManagerPtr shutdown_manager_;
};

class UnaryUnaryClient_ServerNotExist final
{
private:
  using Factory = test::TestService_HandlerUnaryUnary_Factory;
  using Config = Client::Config;
  using Logger_var = Logging::Logger_var;
  using Impl = UnaryUnaryClient_ServerNotExistImpl;

public:
  UnaryUnaryClient_ServerNotExist(
    const Common::ShutdownManagerPtr& shutdown_manager,
    const Config& config,
    const Logger_var& logger)
    : shutdown_manager_(shutdown_manager),
      factory_(std::make_unique<Factory>(config, logger))
  {
  }

  ~UnaryUnaryClient_ServerNotExist() = default;

  void start()
  {
    auto request = std::make_unique<test::Request>();
    request->set_message(kRequestOk);
    factory_->create(
      std::make_shared<Impl>(shutdown_manager_),
      std::move(request));
  }

private:
  const Common::ShutdownManagerPtr shutdown_manager_;

  const std::unique_ptr<Factory> factory_;
};

} // namespace

TEST_F(GrpcFixtureUnaryUnary_Client, UnaryUnary_Client_ServerNotExist)
{
  for (std::size_t i = 1; i <= 100; ++i)
  {
    Common::ShutdownManagerPtr shutdown_manager =
      std::make_shared<Common::ShutdownManager>();
    Client::Config client_config;
    client_config.endpoint =
      "127.0.0.1:" + std::to_string(port_);
    UnaryUnaryClient_ServerNotExist client(
      shutdown_manager,
      client_config,
      logger_);
    client.start();
    shutdown_manager->wait();
  }
}

TEST_F(GrpcFixtureUnaryUnary_Client, UnaryUnary_Client_NoAction)
{
  for (std::size_t i = 1; i <= 100; ++i)
  {
    Common::ShutdownManagerPtr shutdown_manager =
      std::make_shared<Common::ShutdownManager>();
    Client::Config client_config;
    client_config.endpoint =
      "127.0.0.1:" + std::to_string(port_);
    UnaryUnaryClient_ServerNotExist client(
      shutdown_manager,
      client_config,
      logger_);
  }
  EXPECT_TRUE(true);
}