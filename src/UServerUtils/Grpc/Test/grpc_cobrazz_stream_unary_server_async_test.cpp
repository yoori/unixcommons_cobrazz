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

const std::size_t kNumberRequest = 10;
const std::string kRequestOk = "Ok";
const std::size_t kNumberRequestClose = 2;
const std::string kRequestServerClose = "ServerClose";
const std::size_t kNumberRequestException = 2;
const std::string kRequestServerException = "ServerException";

std::atomic<int> kCounterHandlerStreamUnary{0};

class StreamUnaryHandler
  : public test::TestService_HandlerStreamUnary_Handler
{
public:
  void on_request(const test::Request& request) override
  {
    kCounterHandlerStreamUnary.fetch_add(1);
    const auto& message = request.message();
    current_message_ = message;
    if (message == kRequestOk)
    {
      counter_ += 1;
    }
    else if (message == kRequestServerClose)
    {
      counter_ += 1;
      if (counter_ == kNumberRequestClose)
      {
        grpc::Status status(grpc::Status::CANCELLED);
        EXPECT_TRUE(finish(std::move(status)));
      }
    }
    else if (message == kRequestServerException)
    {
      counter_ += 1;
      if (counter_ == kNumberRequestException)
      {
        throw std::runtime_error("Test exception");
      }
    }
  }

  void initialize() override
  {
    kCounterHandlerStreamUnary.fetch_add(1);
  }

  void on_reads_done() override
  {
    kCounterHandlerStreamUnary.fetch_add(1);
    if (current_message_ == kRequestOk)
    {
      EXPECT_EQ(counter_, kNumberRequest);

      auto response = std::make_unique<test::Reply>();
      response->set_message(kRequestOk);
      EXPECT_TRUE(send(std::move(response)));
    }
    else if (current_message_ == kRequestServerException)
    {
      auto response = std::make_unique<test::Reply>();
      response->set_message(kRequestServerException);
      EXPECT_TRUE(send(std::move(response)));
    }
  }

  void on_finish() override
  {
    if (current_message_ == kRequestOk)
    {
      EXPECT_EQ(counter_, kNumberRequest);
    }
    else if (current_message_ == kRequestServerClose)
    {
      EXPECT_EQ(counter_, kNumberRequestClose);
    }
    else if (current_message_ == kRequestServerException)
    {
      EXPECT_EQ(counter_, kNumberRequest);
    }
    kCounterHandlerStreamUnary.fetch_add(1);
  }

private:
  std::string current_message_;

  std::size_t counter_ = 0;
};

class StreamUnaryClient
{
public:
  StreamUnaryClient(const std::shared_ptr<::grpc::Channel>& channel)
    : stub_(test::TestService::NewStub(channel))
  {
  }

  void request(const std::string& data)
  {
    using namespace std::chrono_literals;
    grpc::ClientContext context;
    test::Reply reply;

    auto writer = stub_->HandlerStreamUnary(&context, &reply);
    EXPECT_TRUE(writer);
    if (!writer)
      return;

    if (data == kRequestOk)
    {
      for (std::size_t i = 1; i <= kNumberRequest; ++i)
      {
        test::Request request;
        request.set_message(data);
        const auto status = writer->Write(request);
        EXPECT_TRUE(status);
        if (!status)
        {
          break;
        }

        if (i == kNumberRequest)
        {
          EXPECT_TRUE(writer->WritesDone());
          const auto status = writer->Finish();
          EXPECT_TRUE(status.ok());
          EXPECT_EQ(reply.message(), kRequestOk);
        }
      }
    }
    else if (data == kRequestServerClose)
    {
      bool status = true;
      for (std::size_t i = 1; i <= 20; ++i)
      {
        test::Request request;
        request.set_message(data);
        status = writer->Write(request);
        if (status)
        {
          std::this_thread::sleep_for(300ms);
        }
        else
        {
          break;
        }
      }
      EXPECT_FALSE(status);
    }
    else if (data == kRequestServerException)
    {
      bool status = true;
      for (std::size_t i = 1; i <= kNumberRequest; ++i)
      {
        test::Request request;
        request.set_message(data);
        status = writer->Write(request);
        if (!status)
        {
          break;
        }

        if (i == kNumberRequest)
        {
          EXPECT_TRUE(writer->WritesDone());
          const auto status = writer->Finish();
          EXPECT_TRUE(status.ok());
          EXPECT_EQ(reply.message(), kRequestServerException);
        }
      }
      EXPECT_TRUE(status);
    }
  }

private:
  std::unique_ptr<test::TestService::Stub> stub_;
};

class GrpcFixtureStreamUnary : public testing::Test
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

    server_ = UServerUtils::Grpc::Core::Server::Server_var(
      new UServerUtils::Grpc::Core::Server::Server(
        config,
        logger_.in()));
    server_->register_handler<StreamUnaryHandler>();
  }

  void TearDown() override
  {
  }

  std::size_t port_ = 7778;

  Logging::Logger_var logger_;

  UServerUtils::Grpc::Core::Server::Server_var server_;
};

} // namespace


TEST_F(GrpcFixtureStreamUnary, Ok)
{
  server_->activate_object();

  auto channel =
    grpc::CreateChannel(
      "127.0.0.1:" + std::to_string(port_),
      grpc::InsecureChannelCredentials());

  const std::size_t number_iteration = 100;
  for (std::size_t i = 1; i <= number_iteration; ++i)
  {
    StreamUnaryClient stream_unary_client(channel);
    stream_unary_client.request(kRequestOk);
  }

  EXPECT_EQ(kCounterHandlerStreamUnary.load(), number_iteration * (3 + kNumberRequest));

  server_->deactivate_object();
  server_->wait_object();

  kCounterHandlerStreamUnary.exchange(0);
}

TEST_F(GrpcFixtureStreamUnary, ServerClose)
{
  server_->activate_object();

  auto channel =
    grpc::CreateChannel(
      "127.0.0.1:" + std::to_string(port_),
      grpc::InsecureChannelCredentials());

  StreamUnaryClient stream_unary_client(channel);
  stream_unary_client.request(kRequestServerClose);

  EXPECT_EQ(kCounterHandlerStreamUnary.load(), 3 + kNumberRequestClose);

  server_->deactivate_object();
  server_->wait_object();

  kCounterHandlerStreamUnary.exchange(0);
}

TEST_F(GrpcFixtureStreamUnary, ServerException)
{
  server_->activate_object();

  auto channel =
    grpc::CreateChannel(
      "127.0.0.1:" + std::to_string(port_),
      grpc::InsecureChannelCredentials());

  StreamUnaryClient stream_unary_client(channel);
  stream_unary_client.request(kRequestServerException);

  EXPECT_EQ(kCounterHandlerStreamUnary.load(), 3 + kNumberRequest);

  server_->deactivate_object();
  server_->wait_object();

  kCounterHandlerStreamUnary.exchange(0);
}