// GTEST
#include <gtest/gtest.h>

// PROTO
#include "test_service.cobrazz.pb.hpp"
#include "test.grpc.pb.h"

// GRPC
#include <grpcpp/grpcpp.h>

// STD
#include <atomic>

// THIS
#include <Logger/Logger.hpp>
#include <Logger/StreamLogger.hpp>
#include <UServerUtils/Grpc/Core/Server/Config.hpp>
#include <UServerUtils/Grpc/Core/Server/Server.hpp>

namespace
{

const std::string kRequestOk = "Ok";
const std::string kRequestFinish = "Finish";
const std::string kRequestException = "Exception";

const grpc::StatusCode kStatusCodeFinish = grpc::StatusCode::INTERNAL;
const std::string kStatusCodeFinishErrorMessage = "error_message";
const std::string kStatusCodeFinishErrorDetails = "error_details";

std::atomic<int> kCounterHandlerUnaryUnary{0};

class UnaryUnaryHandler final
  : public test::TestService_HandlerUnaryUnary_Handler
{
public:
  void on_request(const test::Request& request) override
  {
    kCounterHandlerUnaryUnary.fetch_add(1);
    const auto& message = request.message();
    if (message == kRequestOk)
    {
      auto response = std::make_unique<test::Reply>();
      response->set_message(message);
      send(std::move(response));
    }
    else if (message == kRequestFinish)
    {
      grpc::Status status(
        kStatusCodeFinish,
        kStatusCodeFinishErrorMessage,
        kStatusCodeFinishErrorDetails);
      finish(std::move(status));
    }
    else if (message == kRequestException)
    {
      throw std::runtime_error("Test exception");
    }
    else
    {
      throw std::runtime_error("Unknown message");
    }
  }

  void initialize() override
  {
    kCounterHandlerUnaryUnary.fetch_add(1);
  }

  void on_reads_done() override
  {
    kCounterHandlerUnaryUnary.fetch_add(1);
  }

  void on_finish() override
  {
    kCounterHandlerUnaryUnary.fetch_add(1);
  }
};

class UnaryUnaryClient final
{
public:
  UnaryUnaryClient(
    const std::shared_ptr<::grpc::Channel>& channel)
      : stub_(test::TestService::NewStub(channel))
  {
  }

  void request(const std::string& data)
  {
    grpc::ClientContext context;
    test::Request request;
    request.set_message(data);

    test::Reply reply;
    const grpc::Status status =
      stub_->HandlerUnaryUnary(
        &context,
        request,
        &reply);

    if (data == kRequestOk)
    {
      EXPECT_TRUE(status.ok());
      if (status.ok())
      {
        EXPECT_EQ(reply.message(), data);
      }
    }
    else if (data == kRequestFinish)
    {
      EXPECT_FALSE(status.ok());
      EXPECT_EQ(status.error_code(), kStatusCodeFinish);
      EXPECT_EQ(status.error_message(), kStatusCodeFinishErrorMessage);
    }
    else if (data == kRequestException)
    {
      EXPECT_FALSE(status.ok());
      EXPECT_EQ(status.error_code(), grpc::StatusCode::CANCELLED);
    }
  }

private:
  std::unique_ptr<test::TestService::Stub> stub_;
};

class GrpcFixtureUnaryUnary : public testing::Test
{
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

    server_ =
      UServerUtils::Grpc::Core::Server::Server_var(
        new UServerUtils::Grpc::Core::Server::Server(
          config,
          logger_));
    server_->register_handler<UnaryUnaryHandler>();
  }

  void TearDown() override
  {
  }

  std::size_t port_ = 7778;

  Logging::Logger_var logger_;

  UServerUtils::Grpc::Core::Server::Server_var server_;
};

} // namespace


TEST_F(GrpcFixtureUnaryUnary, OneChannel)
{
  server_->activate_object();

  auto channel =
    grpc::CreateChannel(
      "127.0.0.1:" + std::to_string(port_),
      grpc::InsecureChannelCredentials());

  const std::size_t number_loop = 100;
  for (std::size_t i = 1; i <= number_loop; ++i)
  {
    UnaryUnaryClient unary_unary_client(channel);
    unary_unary_client.request(kRequestOk);
  }

  EXPECT_EQ(kCounterHandlerUnaryUnary.exchange(0), 4 * number_loop);

  server_->deactivate_object();
  server_->wait_object();
}

TEST_F(GrpcFixtureUnaryUnary, DifferentChannels)
{
  server_->activate_object();

  const std::size_t number_loop = 100;
  for (std::size_t i = 1; i <= number_loop; ++i)
  {
    auto channel =
      grpc::CreateChannel(
        "127.0.0.1:" + std::to_string(port_),
        grpc::InsecureChannelCredentials());

    UnaryUnaryClient unary_unary_client(channel);
    unary_unary_client.request(kRequestOk);
  }

  EXPECT_EQ(kCounterHandlerUnaryUnary.exchange(0), 4 * number_loop);

  server_->deactivate_object();
  server_->wait_object();
}

TEST_F(GrpcFixtureUnaryUnary, Exception)
{
  server_->activate_object();

  auto channel =
    grpc::CreateChannel(
      "127.0.0.1:" + std::to_string(port_),
      grpc::InsecureChannelCredentials());

  UnaryUnaryClient unary_unary_client(channel);
  unary_unary_client.request(kRequestFinish);

  EXPECT_EQ(kCounterHandlerUnaryUnary.exchange(0), 4);

  server_->deactivate_object();
  server_->wait_object();
}

TEST_F(GrpcFixtureUnaryUnary, Finish)
{
  server_->activate_object();

  auto channel =
    grpc::CreateChannel(
      "127.0.0.1:" + std::to_string(port_),
      grpc::InsecureChannelCredentials());

  UnaryUnaryClient unary_unary_client(channel);
  unary_unary_client.request(kRequestException);

  EXPECT_EQ(kCounterHandlerUnaryUnary.exchange(0), 4);

  server_->deactivate_object();
  server_->wait_object();
}