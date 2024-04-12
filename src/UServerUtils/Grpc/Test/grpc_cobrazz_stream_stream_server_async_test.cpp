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

const std::size_t kNumberRequest = 1000;
const std::string kMessageRequest = "message";

std::atomic<int> kCounterHandlerStreamStream{0};

const grpc::StatusCode kStatusCode = grpc::StatusCode::UNKNOWN;
const std::string kStatusMessage = "message";
const std::string kStatusDetail = "detail";

class StreamStreamHandler
  : public test::TestService_HandlerStreamStream_Handler
{
public:
  void on_request(const test::Request& request) override
  {
    kCounterHandlerStreamStream.fetch_add(1);
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
    kCounterHandlerStreamStream.fetch_add(1);
  }

  void on_reads_done() override
  {
    kCounterHandlerStreamStream.fetch_add(1);
  }

  void on_finish() override
  {
    kCounterHandlerStreamStream.fetch_add(1);
  }

private:
  std::size_t count_ = 0;
};

class StreamStreamClient final
{
public:
  explicit StreamStreamClient(
    const std::shared_ptr<::grpc::Channel>& channel)
    : stub_(test::TestService::NewStub(channel))
  {
  }

  void request(const std::string &data)
  {
    grpc::ClientContext context;

    auto reader_writer = stub_->HandlerStreamStream(&context);
    EXPECT_TRUE(reader_writer);
    if (!reader_writer)
    {
      return;
    }

    std::size_t count = 0;
    while (true)
    {
      count += 1;
      test::Request request;
      request.set_message(kMessageRequest + std::to_string(count));
      reader_writer->Write(request);

      test::Reply reply;
      if (reader_writer->Read(&reply))
      {
        EXPECT_EQ(request.message(), reply.message());
      }
      else
      {
        const auto status = reader_writer->Finish();
        EXPECT_EQ(status.error_code(), kStatusCode);
        EXPECT_EQ(status.error_message(), kStatusMessage);
        break;
      }
    }
  }

private:
  std::unique_ptr<test::TestService::Stub> stub_;
};

class GrpcFixtureStreamStream : public testing::Test
{
public:
  void SetUp() override
  {
    logger_ = new Logging::OStream::Logger(
      Logging::OStream::Config(
        std::cerr,
        Logging::Logger::ERROR));

    UServerUtils::Grpc::Core::Server::Config config;
    config.num_threads = 3;
    config.port = port_;

    server_ = new UServerUtils::Grpc::Core::Server::Server(
      config,
      logger_.in());
    server_->register_handler<StreamStreamHandler>();
  }

  void TearDown() override
  {
  }

  std::size_t port_ = 77778;

  Logging::Logger_var logger_;

  UServerUtils::Grpc::Core::Server::Server_var server_;
};

} // namespace

TEST_F(GrpcFixtureStreamStream, TestStreamStream)
{
  server_->activate_object();

  auto channel = grpc::CreateChannel(
    "127.0.0.1:" + std::to_string(port_),
    grpc::InsecureChannelCredentials());

  StreamStreamClient stream_stream_client(channel);
  stream_stream_client.request(kMessageRequest);

  EXPECT_EQ(kCounterHandlerStreamStream.load(), 3 + kNumberRequest);

  server_->deactivate_object();
  server_->wait_object();

  kCounterHandlerStreamStream.exchange(0);
}