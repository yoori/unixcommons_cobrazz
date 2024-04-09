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
#include <UServerUtils/Grpc/Core/Common/ShutdownManager.hpp>
#include <UServerUtils/Grpc/Core/Client/Config.hpp>
#include <UServerUtils/Grpc/Core/Server/Config.hpp>
#include <UServerUtils/Grpc/Core/Server/Server.hpp>

namespace
{

const std::size_t kNumberRequest = 10;
const std::string kMessageRequest = "message";

const grpc::StatusCode kStatusCode = grpc::StatusCode::UNKNOWN;
const std::string kStatusMessage = "message";
const std::string kStatusDetail = "detail";

std::atomic<int> kCounterClientStreamStream{0};

namespace Client = UServerUtils::Grpc::Core::Client;
namespace Common = UServerUtils::Grpc::Core::Common;

class StreamStreamHandler_ServerFinish final
  : public test::TestService_HandlerStreamStream_Handler
{
public:
  StreamStreamHandler_ServerFinish() = default;

  ~StreamStreamHandler_ServerFinish() = default;

  void on_request(const test::Request& request) override
  {
    count_ += 1;
    const auto& message = request.message();
    EXPECT_EQ(message, kMessageRequest + std::to_string(count_));

    if (count_ == kNumberRequest)
    {
      grpc::Status status(kStatusCode, kStatusMessage, kStatusDetail);
      finish(std::move(status));
    }
    else
    {
      auto response = std::make_unique<test::Reply>();
      response->set_message(message);
      send(std::move(response));
    }
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

private:
  std::size_t count_ = 0;
};

class StreamStreamClient_ServerFinishImpl final:
  public test::TestService_HandlerStreamStream_ClientObserver
{
public:
  using WriterStatus = Client::WriterStatus;

public:
  StreamStreamClient_ServerFinishImpl(
    const Common::ShutdownManagerPtr& shutdown_manager,
    const std::string& data)
    : shutdown_manager_(shutdown_manager),
      data_(data)
  {
  }

  ~StreamStreamClient_ServerFinishImpl() override = default;

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
    kCounterClientStreamStream.fetch_add(1);
    EXPECT_TRUE(ok);
  }

  void on_read(test::Reply&& response) override
  {
    kCounterClientStreamStream.fetch_add(1);
    EXPECT_EQ(response.message(), data_ + std::to_string(counter_));
    counter_ += 1;

    auto request = std::make_unique<test::Request>();
    request->set_message(data_ + std::to_string(counter_));
    auto status = writer_->write(std::move(request));
    EXPECT_EQ(status, WriterStatus::Ok);
  }

  void on_finish(grpc::Status&& status) override
  {
    kCounterClientStreamStream.fetch_add(1);
    EXPECT_EQ(counter_, kNumberRequest);
    EXPECT_EQ(status.error_code(), kStatusCode);
    EXPECT_EQ(status.error_details(), kStatusDetail);
    EXPECT_EQ(status.error_message(), kStatusMessage);
    shutdown_manager_->shutdown();
  }

private:
  Common::ShutdownManagerPtr shutdown_manager_;

  const std::string data_;

  WriterPtr writer_;

  std::size_t counter_ = 1;
};

class StreamStreamClient_ServerFinish final
{
private:
  using Factory = test::TestService_HandlerStreamStream_Factory;
  using Config = Client::Config;
  using Logger = Logging::Logger;
  using Impl = StreamStreamClient_ServerFinishImpl;
  using WriterStatus = Client::WriterStatus;

public:
  StreamStreamClient_ServerFinish(
    const Config& config,
    Logger* logger,
    const Common::ShutdownManagerPtr& shutdown_manager,
    const std::string& data)
    : impl_(std::make_unique<Impl>(shutdown_manager, data)),
      factory_(std::make_unique<Factory>(config, logger)),
      data_(data)
  {
  }

  ~StreamStreamClient_ServerFinish() = default;

  void start()
  {
    auto writer = factory_->create(*impl_);
    auto request = std::make_unique<test::Request>();
    request->set_message(data_ + std::to_string(1));
    auto status = writer->write(std::move(request));
    EXPECT_EQ(status, WriterStatus::Ok);
  }

private:
  std::unique_ptr<Impl> impl_;

  std::unique_ptr<Factory> factory_;

  const std::string data_;
};

class GrpcFixtureStreamStream_Client_ServerFinish : public testing::Test
{
public:
  using ClientPtr = std::shared_ptr<StreamStreamClient_ServerFinish>;

public:
  void SetUp() override
  {
    logger_ = Logging::Logger_var(
      new Logging::OStream::Logger(
        Logging::OStream::Config(
          std::cerr,
          Logging::Logger::ERROR)));

    UServerUtils::Grpc::Core::Server::Config config;
    config.num_threads = 3;
    config.port = port_;

    server_ = UServerUtils::Grpc::Core::Server::Server_var(
      new UServerUtils::Grpc::Core::Server::Server(
        config,
        logger_.in()));
    server_->register_handler<StreamStreamHandler_ServerFinish>();
  }

  void TearDown() override
  {
  }

  std::size_t port_ = 7779;

  Logging::Logger_var logger_;

  UServerUtils::Grpc::Core::Server::Server_var server_;
};

} // namespace

TEST_F(GrpcFixtureStreamStream_Client_ServerFinish, TestStreamStream_Client_ServerFinish)
{
  server_->activate_object();

  Common::ShutdownManagerPtr shutdown_manager =
    std::make_shared<Common::ShutdownManager>();
  Client::Config client_config;
  client_config.endpoint =
    "127.0.0.1:" + std::to_string(port_);
  auto client = std::make_shared<StreamStreamClient_ServerFinish>(
    client_config,
    logger_,
    shutdown_manager,
    kMessageRequest);
  client->start();
  shutdown_manager->wait();

  server_->deactivate_object();
  server_->wait_object();

  EXPECT_EQ(kCounterClientStreamStream.exchange(0), 1 + kNumberRequest);
}

namespace
{

class StreamStreamClient_ServerNotExistImpl final :
  public test::TestService_HandlerStreamStream_ClientObserver
{
public:
  using WriterStatus = Client::WriterStatus;

public:
  StreamStreamClient_ServerNotExistImpl(
    const Common::ShutdownManagerPtr& shutdown_manager)
    : shutdown_manager_(shutdown_manager)
  {
  }

  ~StreamStreamClient_ServerNotExistImpl() override = default;

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
    kCounterClientStreamStream.fetch_add(1);
    EXPECT_FALSE(ok);
  }

  void on_read(test::Reply&& response) override
  {
    kCounterClientStreamStream.fetch_add(1);
  }

  void on_finish(grpc::Status&& status) override
  {
    kCounterClientStreamStream.fetch_add(1);
    EXPECT_TRUE(true);
    shutdown_manager_->shutdown();
  }

private:
  Common::ShutdownManagerPtr shutdown_manager_;

  WriterPtr writer_;
};

class StreamStreamClient_ServerNotExist final
{
private:
  using Factory = test::TestService_HandlerStreamStream_Factory;
  using Config = Client::Config;
  using Logger_var = Logging::Logger_var;
  using WriterStatus = Client::WriterStatus;
  using Impl = StreamStreamClient_ServerNotExistImpl;

public:
  StreamStreamClient_ServerNotExist(
    const Config &config,
    const Logger_var &logger,
    const Common::ShutdownManagerPtr& shutdown_manager)
    : impl_(std::make_unique<Impl>(shutdown_manager)),
      factory_(std::make_unique<Factory>(config, logger))
  {
  }

  ~StreamStreamClient_ServerNotExist() = default;

  void start()
  {
    auto writer = factory_->create(*impl_);
    auto request = std::make_unique<test::Request>();
    request->set_message(kMessageRequest);
    auto status = writer->write(std::move(request));
    EXPECT_EQ(status, WriterStatus::Ok);
  }

private:
  std::unique_ptr<Impl> impl_;

  std::unique_ptr<Factory> factory_;
};

} // namespace

TEST_F(GrpcFixtureStreamStream_Client_ServerFinish, TestStreamStream_Client_ServerNotExist)
{
  Client::Config client_config;
  client_config.endpoint =
    "127.0.0.1:" + std::to_string(port_);

  for (std::size_t i = 1; i <= 100; ++i)
  {
    Common::ShutdownManagerPtr shutdown_manager =
      std::make_shared<Common::ShutdownManager>();

    StreamStreamClient_ServerNotExist client(
      client_config,
      logger_,
      shutdown_manager);
    client.start();
    shutdown_manager->wait();
  }
}

TEST_F(GrpcFixtureStreamStream_Client_ServerFinish, TestStreamStream_Client_NoAction)
{
  for (std::size_t i = 1; i <= 100; ++i)
  {
    Common::ShutdownManagerPtr shutdown_manager =
      std::make_shared<Common::ShutdownManager>();
    Client::Config client_config;
    client_config.endpoint =
      "127.0.0.1:" + std::to_string(port_);
    StreamStreamClient_ServerNotExist client(
      client_config,
      logger_,
      shutdown_manager);
  }
}

namespace
{

class StreamStreamHandler_ClientFinish final
  : public test::TestService_HandlerStreamStream_Handler
{
public:
  StreamStreamHandler_ClientFinish() = default;

  ~StreamStreamHandler_ClientFinish() = default;

  void on_request(const test::Request& request) override
  {
    count_ += 1;
    const auto& message = request.message();
    EXPECT_EQ(message, kMessageRequest + std::to_string(count_));

    auto response = std::make_unique<test::Reply>();
    response->set_message(message);
    send(std::move(response));
  }

  void initialize() override
  {
  }

  void on_reads_done() override
  {
    grpc::Status status(kStatusCode, kStatusMessage, kStatusDetail);
    finish(std::move(status));
  }

  void on_finish() override
  {
  }

private:
  std::size_t count_ = 0;
};

class StreamStreamClient_ClientFinishImpl final:
  public test::TestService_HandlerStreamStream_ClientObserver
{
public:
  using WriterStatus = Client::WriterStatus;

public:
  StreamStreamClient_ClientFinishImpl(
    const Common::ShutdownManagerPtr& shutdown_manager,
    const std::string& data)
    : shutdown_manager_(shutdown_manager),
      data_(data)
  {
  }

  ~StreamStreamClient_ClientFinishImpl() override = default;

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
    kCounterClientStreamStream.fetch_add(1);
    EXPECT_TRUE(ok);
  }

  void on_read(test::Reply&& response) override
  {
    kCounterClientStreamStream.fetch_add(1);
    EXPECT_EQ(response.message(), data_ + std::to_string(counter_));
    counter_ += 1;

    if (counter_ == kNumberRequest)
    {
      const auto status = writer_->writes_done();
      EXPECT_EQ(status, WriterStatus::Ok);
    }
    else
    {
      auto request = std::make_unique<test::Request>();
      request->set_message(data_ + std::to_string(counter_));
      const auto status = writer_->write(std::move(request));
      EXPECT_EQ(status, WriterStatus::Ok);
    }
  }

  void on_finish(grpc::Status&& status) override
  {
    kCounterClientStreamStream.fetch_add(1);

    EXPECT_EQ(counter_, kNumberRequest);
    EXPECT_EQ(status.error_code(), kStatusCode);
    EXPECT_EQ(status.error_details(), kStatusDetail);
    EXPECT_EQ(status.error_message(), kStatusMessage);

    shutdown_manager_->shutdown();
  }

private:
  Common::ShutdownManagerPtr shutdown_manager_;

  const std::string data_;

  WriterPtr writer_;

  std::size_t counter_ = 1;
};

class StreamStreamClient_ClientFinish final
{
private:
  using Factory = test::TestService_HandlerStreamStream_Factory;
  using Config = Client::Config;
  using Logger_var = Logging::Logger_var;
  using WriterStatus = Client::WriterStatus;
  using Impl = StreamStreamClient_ClientFinishImpl;

public:
  StreamStreamClient_ClientFinish(
    const Config& config,
    const Logger_var& logger,
    const Common::ShutdownManagerPtr& shutdown_manager,
    const std::string& data)
    : impl_(std::make_unique<Impl>(shutdown_manager, data)),
      factory_(std::make_unique<Factory>(config, logger)),
      data_(data)
  {
  }

  ~StreamStreamClient_ClientFinish()
  {
    EXPECT_EQ(factory_->size(), 0);
  }

  void start()
  {
    auto writer = factory_->create(*impl_);
    auto request = std::make_unique<test::Request>();
    request->set_message(data_ + std::to_string(1));
    auto status = writer->write(std::move(request));
    EXPECT_EQ(status, WriterStatus::Ok);
  }

private:
  std::unique_ptr<Impl> impl_;

  std::unique_ptr<Factory> factory_;

  const std::string data_;
};

class GrpcFixtureStreamStream_Client_ClientFinish : public testing::Test
{
public:
  using ClientPtr = std::unique_ptr<StreamStreamClient_ServerFinish>;

public:
  void SetUp() override
  {
    logger_ = Logging::Logger_var(
      new Logging::OStream::Logger(
        Logging::OStream::Config(
          std::cerr,
          Logging::Logger::ERROR)));

    UServerUtils::Grpc::Core::Server::Config config;
    config.num_threads = 3;
    config.port = port_;

    server_ = UServerUtils::Grpc::Core::Server::Server_var(
      new UServerUtils::Grpc::Core::Server::Server(
        config,
        logger_));
    server_->register_handler<StreamStreamHandler_ClientFinish>();
  }

  void TearDown() override
  {
  }

  std::size_t port_ = 7779;

  Logging::Logger_var logger_;

  UServerUtils::Grpc::Core::Server::Server_var server_;
};

} // namespace

TEST_F(GrpcFixtureStreamStream_Client_ClientFinish, TestStreamStream_Client_NoAction)
{
  server_->activate_object();

  Common::ShutdownManagerPtr shutdown_manager =
    std::make_shared<Common::ShutdownManager>();
  Client::Config client_config;
  client_config.endpoint =
    "127.0.0.1:" + std::to_string(port_);
  kCounterClientStreamStream.exchange(0);

  StreamStreamClient_ClientFinish client(
    client_config,
    logger_,
    shutdown_manager,
    kMessageRequest);
  client.start();
  shutdown_manager->wait();

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  server_->deactivate_object();
  server_->wait_object();

  EXPECT_EQ(kNumberRequest + 1, kCounterClientStreamStream.exchange(0));
}