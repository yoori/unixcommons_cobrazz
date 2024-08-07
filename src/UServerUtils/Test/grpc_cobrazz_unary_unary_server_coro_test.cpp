// GTEST
#include <gtest/gtest.h>

// STD
#include <atomic>

// PROTO
#include "test_service.cobrazz.pb.hpp"
#include "test.grpc.pb.h"

// THIS
#include <Logger/Logger.hpp>
#include <Logger/StreamLogger.hpp>
#include <UServerUtils/Grpc/Server/ConfigCoro.hpp>
#include <UServerUtils/Grpc/Server/ServerBuilder.hpp>
#include <UServerUtils/ComponentsBuilder.hpp>
#include <UServerUtils/Manager.hpp>

using namespace UServerUtils;

namespace
{

const std::string kRequestOk = "Ok";
const std::string kRequestOkCheckFinish = "OkCheckFinish";
const std::string kRequestFinish = "Finish";
const std::string kRequestException = "Exception";

const grpc::StatusCode kStatusCodeFinish = grpc::StatusCode::INTERNAL;
const std::string kStatusCodeFinishErrorMessage = "error_message";
const std::string kStatusCodeFinishErrorDetails = "error_details";

std::atomic<int> kCountCallObject{0};

class UnaryUnaryCoroPerRpcService final
  : public test::TestService_HandlerUnaryUnary_Service,
    public ReferenceCounting::AtomicImpl
{
public:
  UnaryUnaryCoroPerRpcService() = default;

  ~UnaryUnaryCoroPerRpcService() override = default;

   void activate_object() override
   {
     kCountCallObject.fetch_add(1, std::memory_order_relaxed);
     TestService_HandlerUnaryUnary_Service::activate_object();
   }

   void deactivate_object() override
   {
     kCountCallObject.fetch_add(1, std::memory_order_relaxed);
     TestService_HandlerUnaryUnary_Service::deactivate_object();
   }

   void wait_object() override
   {
     kCountCallObject.fetch_add(1, std::memory_order_relaxed);
     TestService_HandlerUnaryUnary_Service::wait_object();
   }

  void handle(const Reader& reader) override
  {
    const auto data = reader.read();

    EXPECT_EQ(data.status, ReadStatus::Read);
    if (data.status != ReadStatus::Read)
      return;

    const auto& request = data.request;
    EXPECT_TRUE(request);
    if (!request)
      return;

    const auto& writer = data.writer;
    EXPECT_TRUE(writer);
    if (!writer)
      return;

    const auto& message = request->message();
    if (kRequestOk == message)
    {
      auto response = std::make_unique<Response>();
      response->set_message(message);
      const auto status_write = writer->write(std::move(response));
      EXPECT_EQ(status_write, WriterStatus::Ok);
    }
    else if (kRequestOkCheckFinish == message)
    {
      auto response = std::make_unique<Response>();
      response->set_message(message);
      const auto status_write = writer->write(std::move(response));
      EXPECT_EQ(status_write, WriterStatus::Ok);

      for (std::size_t i = 1; i <= 100; ++i)
      {
        const auto data = reader.read();
        EXPECT_EQ(data.status, ReadStatus::Finish);
      }
    }
    else if (kRequestFinish == message)
    {
      grpc::Status status(
        kStatusCodeFinish,
        kStatusCodeFinishErrorMessage,
        kStatusCodeFinishErrorDetails);

      const auto status_write = writer->finish(std::move(status));
      EXPECT_EQ(status_write, WriterStatus::Ok);
    }
    else if (kRequestException == message)
    {
      throw std::runtime_error("test exception");
    }
    else
    {
      EXPECT_TRUE(false);
    }
  }
};

using UnaryUnaryCoroPerRpcService_var = ReferenceCounting::SmartPtr<UnaryUnaryCoroPerRpcService>;

class UnaryUnaryCoroPerRpcClient final
{
public:
  explicit UnaryUnaryCoroPerRpcClient(
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
    else if (data == kRequestOkCheckFinish)
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
      EXPECT_EQ(status.error_details(), kStatusCodeFinishErrorDetails);
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

class GrpcFixtureUnaryUnaryCoroPerRpc : public testing::Test
{
public:
  void SetUp() override
  {
    logger_ = new Logging::OStream::Logger(
      Logging::OStream::Config(
        std::cerr,
        Logging::Logger::CRITICAL));

    CoroPoolConfig coro_pool_config;
    EventThreadPoolConfig event_thread_pool_config;
    TaskProcessorConfig main_task_processor_config;
    main_task_processor_config.name = "main_task_processor";
    main_task_processor_config.worker_threads = 3;
    main_task_processor_config.thread_name = "main_tskpr";

    auto task_processor_container_builder =
      std::make_unique<TaskProcessorContainerBuilder>(
        logger_.in(),
        coro_pool_config,
        event_thread_pool_config,
        main_task_processor_config);

    auto init_func = [logger = logger_, port = port_] (
      TaskProcessorContainer& task_processor_container) {
      auto& main_task_processor =
        task_processor_container.get_main_task_processor();

      auto components_builder =
        std::make_unique<ComponentsBuilder>();

      Grpc::Server::ConfigCoro config;
      config.num_threads = 3;
      config.port = port;
      config.max_size_queue = {};

      auto grpc_builder = std::make_unique<Grpc::Server::ServerBuilder>(
        config,
        logger.in());
      auto service = UnaryUnaryCoroPerRpcService_var(
        new UnaryUnaryCoroPerRpcService);
      grpc_builder->add_service(
        service.in(),
        main_task_processor);

      components_builder->add_grpc_cobrazz_server(
        std::move(grpc_builder));

      return components_builder;
    };

    manager_ =
      Manager_var(
        new Manager(
          std::move(task_processor_container_builder),
          std::move(init_func),
          logger_.in()));
  }

  void TearDown() override
  {
  }

  std::size_t port_ = 7778;

  Logging::Logger_var logger_;

  Manager_var manager_;
};

} // namespace

TEST_F(GrpcFixtureUnaryUnaryCoroPerRpc, CoroPerRpc)
{
  manager_->activate_object();

  auto channel =
    grpc::CreateChannel(
      "127.0.0.1:" + std::to_string(port_),
      grpc::InsecureChannelCredentials());

  for (std::size_t i = 1; i <= 100; ++i)
  {
    UnaryUnaryCoroPerRpcClient unary_unary_client(channel);
    unary_unary_client.request(kRequestOk);
    unary_unary_client.request(kRequestOkCheckFinish);
    unary_unary_client.request(kRequestFinish);
    unary_unary_client.request(kRequestException);
  }

  UnaryUnaryCoroPerRpcClient unary_unary_client(channel);
  for (std::size_t i = 1; i <= 100; ++i)
  {
    unary_unary_client.request(kRequestOk);
    unary_unary_client.request(kRequestOkCheckFinish);
    unary_unary_client.request(kRequestFinish);
    unary_unary_client.request(kRequestException);
  }

  manager_->deactivate_object();
  manager_->wait_object();

  EXPECT_EQ(kCountCallObject.exchange(0), 3);
}

namespace
{

const int kNumberOkRequest = 100;
std::atomic<int> kCounterServiceUnaryUnary{0};

class UnaryUnaryCoroSetService final
  : public test::TestService_HandlerUnaryUnary_Service,
    public ReferenceCounting::AtomicImpl
{
public:
  UnaryUnaryCoroSetService() = default;

  ~UnaryUnaryCoroSetService() override = default;

  void handle(const Reader& reader) override
  {
    while (true)
    {
      const auto data = reader.read();
      const auto status = data.status;
      if (status == ReadStatus::Finish)
      {
        break;
      }
      else if (status == ReadStatus::Read)
      {
        auto& request = data.request;
        EXPECT_TRUE(request);
        if (!request)
          continue;

        auto& writer = data.writer;
        EXPECT_TRUE(writer);
        if (!writer)
          continue;

        const auto& message = request->message();
        if (message == kRequestOk)
        {
          auto response = std::make_unique<Response>();
          response->set_message(message);
          const auto writer_status = writer->write(std::move(response));
          EXPECT_EQ(writer_status, WriterStatus::Ok);

          kCounterServiceUnaryUnary.fetch_add(1, std::memory_order_relaxed);
        }
        else if (message == kRequestFinish)
        {
          grpc::Status status(
            kStatusCodeFinish,
            kStatusCodeFinishErrorMessage,
            kStatusCodeFinishErrorDetails);

          const auto status_write = writer->finish(std::move(status));
          EXPECT_EQ(status_write, WriterStatus::Ok);
        }
        else if (message == kRequestException)
        {
          throw std::runtime_error("test exception");
        }
        else
        {
          EXPECT_TRUE(false);
        }
      }
      else
      {
        EXPECT_TRUE(false);
      }
    }
  }
};

using UnaryUnaryCoroSetService_var = ReferenceCounting::SmartPtr<UnaryUnaryCoroSetService>;

class UnaryUnaryCoroSetClient final
{
public:
  explicit UnaryUnaryCoroSetClient(
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
      EXPECT_EQ(status.error_details(), kStatusCodeFinishErrorDetails);
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

class GrpcFixtureUnaryUnaryCoroSet : public testing::Test
{
public:
  void SetUp() override
  {
    logger_ = new Logging::OStream::Logger(
      Logging::OStream::Config(
        std::cerr,
        Logging::Logger::CRITICAL));

    CoroPoolConfig coro_pool_config;
    EventThreadPoolConfig event_thread_pool_config;
    TaskProcessorConfig main_task_processor_config;
    main_task_processor_config.name = "main_task_processor";
    main_task_processor_config.worker_threads = 3;
    main_task_processor_config.thread_name = "main_tskpr";

    auto task_processor_container_builder = std::make_unique<TaskProcessorContainerBuilder>(
      logger_,
      coro_pool_config,
      event_thread_pool_config,
      main_task_processor_config);

    auto init_func = [logger = logger_, port = port_] (
      TaskProcessorContainer& task_processor_container) {
      auto& main_task_processor = task_processor_container.get_main_task_processor();
      auto components_builder = std::make_unique<ComponentsBuilder>();

      Grpc::Server::ConfigCoro config;
      config.num_threads = 3;
      config.port = port;
      config.max_size_queue = {};

      auto grpc_builder = std::make_unique<Grpc::Server::ServerBuilder>(
        config,
        logger);
      auto service = UnaryUnaryCoroSetService_var(
        new UnaryUnaryCoroSetService);
      grpc_builder->add_service(
        service.in(),
        main_task_processor,
        5);

      components_builder->add_grpc_cobrazz_server(
        std::move(grpc_builder));

      return components_builder;
    };

    manager_ = new Manager(
      std::move(task_processor_container_builder),
      std::move(init_func),
      logger_);
  }

  void TearDown() override
  {
  }

  std::size_t port_ = 7778;

  Logging::Logger_var logger_;

  Manager_var manager_;
};

} // // namespace

TEST_F(GrpcFixtureUnaryUnaryCoroSet, CoroSet)
{
  manager_->activate_object();

  auto channel = grpc::CreateChannel(
    "127.0.0.1:" + std::to_string(port_),
    grpc::InsecureChannelCredentials());

  for (std::size_t i = 1; i <= kNumberOkRequest; ++i)
  {
    UnaryUnaryCoroSetClient unary_unary_client(channel);
    unary_unary_client.request(kRequestOk);
  }

  EXPECT_EQ(kCounterServiceUnaryUnary.exchange(0), kNumberOkRequest);

  UnaryUnaryCoroSetClient unary_unary_client(channel);
  for (std::size_t i = 1; i <= kNumberOkRequest; ++i)
  {
    unary_unary_client.request(kRequestOk);
  }

  EXPECT_EQ(kCounterServiceUnaryUnary.exchange(0), kNumberOkRequest);

  for (std::size_t i = 1; i <= 100; ++i)
  {
    UnaryUnaryCoroSetClient unary_unary_client(channel);
    unary_unary_client.request(kRequestException);
    unary_unary_client.request(kRequestFinish);
  }

  UnaryUnaryCoroSetClient unary_unary_client2(channel);
  for (std::size_t i = 1; i <= 100; ++i)
  {
    unary_unary_client2.request(kRequestException);
    unary_unary_client2.request(kRequestFinish);
  }

  manager_->deactivate_object();
  manager_->wait_object();
}