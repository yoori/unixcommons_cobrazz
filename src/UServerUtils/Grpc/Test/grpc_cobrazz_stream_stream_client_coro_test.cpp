// GTEST
#include <gtest/gtest.h>

// PROTO
#include "test_coro_client_client.cobrazz.pb.hpp"
#include "test_coro_client_service.cobrazz.pb.hpp"

// GRPC
#include <grpcpp/grpcpp.h>

// STD
#include <atomic>

// THIS
#include <Logger/Logger.hpp>
#include <Logger/StreamLogger.hpp>
#include <UServerUtils/Grpc/CobrazzClientFactory.hpp>
#include <UServerUtils/Grpc/CobrazzServerBuilder.hpp>
#include <UServerUtils/Grpc/ComponentsBuilder.hpp>
#include <UServerUtils/Grpc/Manager.hpp>
#include <UServerUtils/Grpc/Core/Common/ShutdownManager.hpp>
#include <UServerUtils/Grpc/Core/Common/ThreadGuard.hpp>
#include <UServerUtils/Grpc/Core/Server/ConfigCoro.hpp>

using namespace UServerUtils::Grpc;

namespace
{

const std::string kMessageRequest = "message";

std::atomic<std::size_t> kCountFinish{0};
std::atomic<std::size_t> kCountInitialize{0};
std::atomic<std::size_t> kCountReadsDone{0};
std::atomic<std::size_t> kCountRead{0};
std::atomic<std::size_t> kCountCreateService{0};
std::atomic<std::size_t> kCountDestroyService{0};

class StreamStreamCoro_ClientTest_Success final
  : public test_coro::TestCoroService_Handler_Service,
    public ReferenceCounting::AtomicImpl
{
public:
  StreamStreamCoro_ClientTest_Success()
  {
    kCountCreateService.fetch_add(1);
  }

  ~StreamStreamCoro_ClientTest_Success() override
  {
    kCountDestroyService.fetch_add(1);
  }

  void handle(const Reader& reader) override
  {
    while(true)
    {
      const auto data = reader.read();
      const auto status = data.status;
      if (status == ReadStatus::Finish)
      {
        break;
      }
      else if (status == ReadStatus::Initialize)
      {
        kCountInitialize.fetch_add(1);
      }
      else if (status == ReadStatus::ReadsDone)
      {
        kCountReadsDone.fetch_add(1);
      }
      else if (status == ReadStatus::RpcFinish)
      {
        kCountFinish.fetch_add(1);
      }
      else if (status == ReadStatus::Read)
      {
        kCountRead.fetch_add(1);
        auto& request = data.request;
        EXPECT_TRUE(request);
        if (!request)
          continue;

        auto& writer = data.writer;
        EXPECT_TRUE(writer);
        if (!writer)
          continue;

        auto response = std::make_unique<Response>();
        response->set_message(request->message());
        response->set_id_request_grpc(request->id_request_grpc());
        const auto writer_status = writer->write(std::move(response));
        EXPECT_EQ(writer_status, WriterStatus::Ok);
      }
      else
      {
        EXPECT_TRUE(false);
      }
    }
  }
};

using StreamStreamCoro_ClientTest_Success_var =
  ReferenceCounting::SmartPtr<StreamStreamCoro_ClientTest_Success>;

class GrpcFixtureStreamStreamCoro_ClientTest_Success
  : public testing::Test
{
public:
  void SetUp() override
  {
    logger_ = Logging::Logger_var(
      new Logging::OStream::Logger(
        Logging::OStream::Config(
          std::cerr,
          Logging::Logger::ERROR)));

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

      Core::Server::ConfigCoro config;
      config.num_threads = 3;
      config.port = port;
      config.max_size_queue = {};

      auto grpc_builder =
        std::make_unique<GrpcCobrazzServerBuilder>(
          config,
          logger.in());
      auto service =
        StreamStreamCoro_ClientTest_Success_var(
          new StreamStreamCoro_ClientTest_Success);
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

  std::size_t port_ = 7779;

  Logging::Logger_var logger_;

  Manager_var manager_;
};

} // namespace

TEST_F(GrpcFixtureStreamStreamCoro_ClientTest_Success, Success)
{
  using ConfigPoolCoro =
    UServerUtils::Grpc::Core::Client::ConfigPoolCoro;

  manager_->activate_object();

  auto& task_processor = manager_->get_main_task_processor();

  ConfigPoolCoro config;
  config.endpoint = "127.0.0.1:" + std::to_string(port_);
  config.number_async_client = 17;
  config.number_threads = 7;

  const std::size_t number_cycle = 100;
  const std::size_t number_request = 10;
  for (std::size_t i = 1; i <= number_cycle; ++i)
  {
    auto pool = GrpcCobrazzPoolClientFactory::create<
      test_coro::TestCoroService_Handler_ClientPool>(
        logger_.in(),
        config,
        task_processor);

    for (std::size_t i = 1; i <= number_request; ++i)
    {
      auto request = std::make_unique<test_coro::Request>();
      const auto message = kMessageRequest + std::to_string(i);
      request->set_message(message);
      auto result = pool->write(std::move(request), 2000);
      EXPECT_EQ(result.status, UServerUtils::Grpc::Core::Client::Status::Ok);
      if (result.status == UServerUtils::Grpc::Core::Client::Status::Ok)
      {
        auto& response = result.response;
        EXPECT_TRUE(response);
        EXPECT_EQ(response->message(), message);
      }
    }
  }

  manager_->deactivate_object();
  manager_->wait_object();
  manager_.reset();

  EXPECT_EQ(kCountInitialize, kCountFinish);
  EXPECT_EQ(kCountInitialize, kCountReadsDone);
  EXPECT_EQ(kCountRead, number_cycle * number_request);
  EXPECT_EQ(kCountCreateService.exchange(0), kCountDestroyService.exchange(0));
}

TEST_F(GrpcFixtureStreamStreamCoro_ClientTest_Success, Success_MultiThread)
{
  using ConfigPoolCoro =
    UServerUtils::Grpc::Core::Client::ConfigPoolCoro;

  manager_->activate_object();

  auto& task_processor = manager_->get_main_task_processor();

  ConfigPoolCoro config;
  config.endpoint = "127.0.0.1:" + std::to_string(port_);
  config.number_async_client = 20;
  config.number_threads = 7;

  const std::size_t number_request = 500;
  const std::size_t number_threads = 100;
  const std::size_t number_cycle = 3;
  for (std::size_t k = 1; k <= number_cycle; ++k)
  {
    auto pool = GrpcCobrazzPoolClientFactory::create<
      test_coro::TestCoroService_Handler_ClientPool>(
      logger_.in(),
      config,
      task_processor);

    UServerUtils::Grpc::Core::Common::ThreadsGuard threads_guard;
    UServerUtils::Grpc::Core::Common::ShutdownManagerPtr manager(
      new UServerUtils::Grpc::Core::Common::ShutdownManager);
    for (std::size_t i = 1; i <= number_threads; ++i)
    {
      threads_guard.add([manager, number_request, pool] () {
        manager->wait();
        for (std::size_t i = 1; i <= number_request; ++i)
        {
          auto request = std::make_unique<test_coro::Request>();
          const auto message = kMessageRequest + std::to_string(i);
          request->set_message(message);
          auto result = pool->write(std::move(request), 2000);
          EXPECT_EQ(result.status, UServerUtils::Grpc::Core::Client::Status::Ok);
          if (result.status == UServerUtils::Grpc::Core::Client::Status::Ok)
          {
            auto& response = result.response;
            EXPECT_TRUE(response);
            EXPECT_EQ(response->message(), message);
          }
        }
      });
    }
    manager->shutdown();
  }

  manager_->deactivate_object();
  manager_->wait_object();
  manager_.reset();

  EXPECT_EQ(kCountCreateService.exchange(0), kCountDestroyService.exchange(0));
}

namespace
{

class StreamStreamCoro_ClientTest_Timeout final
  : public test_coro::TestCoroService_Handler_Service,
    public ReferenceCounting::AtomicImpl
{
public:
  StreamStreamCoro_ClientTest_Timeout() = default;

  ~StreamStreamCoro_ClientTest_Timeout() override = default;

  void handle(const Reader& reader) override
  {
    while(true)
    {
      const auto data = reader.read();
      const auto status = data.status;
      if (status == ReadStatus::Finish)
      {
        break;
      }
    }
  }
};

using StreamStreamCoro_ClientTest_Timeout_var =
  ReferenceCounting::SmartPtr<StreamStreamCoro_ClientTest_Timeout>;

class GrpcFixtureStreamStreamCoro_ClientTest_Timeout
  : public testing::Test
{
public:
  void SetUp() override
  {
    logger_ = Logging::Logger_var(
      new Logging::OStream::Logger(
        Logging::OStream::Config(
          std::cerr,
          Logging::Logger::ERROR)));

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

      Core::Server::ConfigCoro config;
      config.num_threads = 3;
      config.port = port;
      config.max_size_queue = {};

      auto grpc_builder =
        std::make_unique<GrpcCobrazzServerBuilder>(
          config,
          logger.in());
      auto service =
        StreamStreamCoro_ClientTest_Timeout_var(
          new StreamStreamCoro_ClientTest_Timeout);
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

  std::size_t port_ = 7779;

  Logging::Logger_var logger_;

  Manager_var manager_;
};

} // namespace

TEST_F(GrpcFixtureStreamStreamCoro_ClientTest_Timeout, Timeout)
{
  using ConfigPoolCoro =
    UServerUtils::Grpc::Core::Client::ConfigPoolCoro;

  manager_->activate_object();

  auto& task_processor = manager_->get_main_task_processor();

  ConfigPoolCoro config;
  config.endpoint = "127.0.0.1:" + std::to_string(port_);
  config.number_async_client = 17;
  config.number_threads = 7;

  auto pool = GrpcCobrazzPoolClientFactory::create<
    test_coro::TestCoroService_Handler_ClientPool>(
    logger_.in(),
    config,
    task_processor);

  const std::size_t number_request = 10;
  for (std::size_t i = 1; i <= number_request; ++i)
  {
    auto request = std::make_unique<test_coro::Request>();
    const auto message = kMessageRequest + std::to_string(i);
    request->set_message(message);
    auto result = pool->write(std::move(request), 100);
    EXPECT_EQ(result.status, UServerUtils::Grpc::Core::Client::Status::Timeout);
  }
  pool.reset();

  manager_->deactivate_object();
  manager_->wait_object();
}

namespace
{

class GrpcFixtureStreamStreamCoro_ClientTest_NotExistingServer
  : public testing::Test
{
public:
  void SetUp() override
  {
    logger_ = Logging::Logger_var(
      new Logging::OStream::Logger(
        Logging::OStream::Config(
          std::cerr,
          Logging::Logger::CRITICAL)));

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
      auto components_builder =
        std::make_unique<ComponentsBuilder>();

      Core::Server::ConfigCoro config;
      config.num_threads = 3;
      config.port = port;
      config.max_size_queue = {};

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

  std::size_t port_ = 7779;

  Logging::Logger_var logger_;

  Manager_var manager_;
};

} // namespace

TEST_F(GrpcFixtureStreamStreamCoro_ClientTest_NotExistingServer, NotExistingServer1)
{
  using ConfigPoolCoro =
    UServerUtils::Grpc::Core::Client::ConfigPoolCoro;

  manager_->activate_object();

  auto& task_processor = manager_->get_main_task_processor();

  ConfigPoolCoro config;
  config.endpoint = "127.0.0.1:" + std::to_string(7779);
  config.number_async_client = 20;
  config.number_threads = 10;

  for (std::size_t k = 1; k <= 500; ++k)
  {
    auto pool = GrpcCobrazzPoolClientFactory::create<
      test_coro::TestCoroService_Handler_ClientPool>(
      logger_.in(),
      config,
      task_processor);
  }

  manager_->deactivate_object();
  manager_->wait_object();
}

TEST_F(GrpcFixtureStreamStreamCoro_ClientTest_NotExistingServer, NotExistingServer2)
{
  using ConfigPoolCoro =
    UServerUtils::Grpc::Core::Client::ConfigPoolCoro;

  manager_->activate_object();

  auto& task_processor = manager_->get_main_task_processor();

  ConfigPoolCoro config;
  config.endpoint = "127.0.0.1:" + std::to_string(7779);
  config.number_async_client = 20;
  config.number_threads = 10;

  const std::size_t number_request = 100;
  for (std::size_t k = 1; k <= 500; ++k)
  {
    auto pool = GrpcCobrazzPoolClientFactory::create<
      test_coro::TestCoroService_Handler_ClientPool>(
      logger_.in(),
      config,
      task_processor);

    for (std::size_t i = 1; i <= number_request; ++i)
    {
      auto request = std::make_unique<test_coro::Request>();
      const auto message = kMessageRequest + std::to_string(i);
      request->set_message(message);
      auto result = pool->write(std::move(request), 2000);
      EXPECT_NE(result.status, UServerUtils::Grpc::Core::Client::Status::Ok);
    }
  }

  manager_->deactivate_object();
  manager_->wait_object();
}