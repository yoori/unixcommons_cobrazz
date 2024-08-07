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
#include <UServerUtils/Grpc/Server/Config.hpp>
#include <UServerUtils/Grpc/Server/Server.hpp>

namespace
{

std::string kMessageRequest = "message";
std::size_t kNumberRequest = 20;
std::atomic<int> kCounterHandlerUnaryStream{0};

const grpc::StatusCode kStatusCode = grpc::StatusCode::UNKNOWN;
const std::string kStatusMessage = "message";
const std::string kStatusDetail = "detail";

class UnaryStreamHandler
  : public test::TestService_HandlerUnaryStream_Handler
{
public:
  void on_request(const test::Request& request) override
  {
    kCounterHandlerUnaryStream.fetch_add(1);
    EXPECT_EQ(request.message(), kMessageRequest);

    for (std::size_t i = 1; i <= kNumberRequest; ++i)
    {
      auto response = std::make_unique<test::Reply>();
      response->set_message(kMessageRequest + std::to_string(i));
      EXPECT_TRUE(send(std::move(response)));
    }

    grpc::Status status(kStatusCode, kStatusMessage, kStatusDetail);
    EXPECT_TRUE(finish(std::move(status)));
  }

  void initialize() override
  {
    kCounterHandlerUnaryStream.fetch_add(1);
  }

  void on_reads_done() override
  {
    kCounterHandlerUnaryStream.fetch_add(1);
  }

  void on_finish() override
  {
    kCounterHandlerUnaryStream.fetch_add(1);
  }
};

class UnaryStreamClient
{
public:
  UnaryStreamClient(
    const std::shared_ptr<::grpc::Channel>& channel)
    : stub_(test::TestService::NewStub(channel))
  {
  }

  void request(const std::string& data)
  {
    grpc::ClientContext context;
    test::Request request;
    request.set_message(data);

    auto reader = stub_->HandlerUnaryStream(&context, request);
    EXPECT_TRUE(reader);
    if (!reader)
    {
      return;
    }

    std::size_t counter = 0;
    while (true)
    {
      test::Reply reply;
      if (reader->Read(&reply))
      {
        counter += 1;
        EXPECT_EQ(reply.message(), kMessageRequest + std::to_string(counter));
      }
      else
      {
        const auto status = reader->Finish();
        EXPECT_EQ(status.error_code(), kStatusCode);
        EXPECT_EQ(status.error_message(), kStatusMessage);
        EXPECT_EQ(status.error_details(), kStatusDetail);
        break;
      }
    }

    EXPECT_EQ(counter, kNumberRequest);
  }

private:
  std::unique_ptr<test::TestService::Stub> stub_;
};

class GrpcFixtureUnaryStream : public testing::Test
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

    server_ = UServerUtils::Grpc::Server::Server_var(
      new UServerUtils::Grpc::Server::Server(
        config,
        logger_.in()));
    server_->register_handler<UnaryStreamHandler>();
  }

  void TearDown() override
  {
  }

  std::size_t port_ = 7778;

  Logging::Logger_var logger_;

  UServerUtils::Grpc::Server::Server_var server_;
};

} // namespace

TEST_F(GrpcFixtureUnaryStream, TestUnaryStream)
{
  server_->activate_object();

  auto channel =
    grpc::CreateChannel(
      "127.0.0.1:" + std::to_string(port_),
      grpc::InsecureChannelCredentials());

  UnaryStreamClient unary_stream_client(channel);
  unary_stream_client.request(kMessageRequest);

  EXPECT_EQ(kCounterHandlerUnaryStream.load(), 4);

  server_->deactivate_object();
  server_->wait_object();

  kCounterHandlerUnaryStream.exchange(0);
}