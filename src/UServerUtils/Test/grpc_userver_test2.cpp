// STD
#include <chrono>
#include <iostream>
#include <deque>

// GTEST
#include <gtest/gtest.h>

// USERVER
#include <userver/engine/sleep.hpp>

// PROTO
#include "test1_service.usrv.pb.hpp"
#include "test2_service.usrv.pb.hpp"
#include "test1_client.usrv.pb.hpp"
#include "test2_client.usrv.pb.hpp"

// THIS
#include <Logger/Logger.hpp>
#include <Logger/StreamLogger.hpp>
#include <UServerUtils/ComponentsBuilder.hpp>
#include <UServerUtils/Manager.hpp>
#include <UServerUtils/Utils.hpp>

namespace
{

const std::string kNameClientTest = "client";

const std::size_t kPortServer1 = 7978;
const std::size_t kPortServer2 = 7979;

const std::string kNameChannelTaskProcessor = "Channel";

const std::string kNameService1 = "Service1";
const std::string kNameService2 = "Service2";

const std::string kRequestService1 = "RequestService1";
const std::string kResponseService1 = "ResponseService1";
bool kCheckService1Activate = false;
bool kCheckService1Deactivate = false;

const std::size_t kCountRequestService2 = 100;
const std::string kRequestService2 = "RequestService2";
const std::string kResponseService2 = "ResponseService2";

class Test1Service final
  : public UServerUtils::Grpc::Test1::TestUnaryServiceBase,
    public ReferenceCounting::AtomicImpl
{
public:
  Test1Service(const std::string& name)
              : name_(name)
  {
    EXPECT_EQ(name_, kNameService1);
  }

  ~Test1Service() override = default;

  void activate_object() override
  {
    UServerUtils::Grpc::Test1::TestUnaryServiceBase::activate_object();
    kCheckService1Activate = true;
  }

  void deactivate_object() override
  {
    UServerUtils::Grpc::Test1::TestUnaryServiceBase::deactivate_object();
    kCheckService1Deactivate = true;
  }

  void say_hello(
    say_helloCall& call,
    ::UServerUtils::Grpc::Test1::Request&& request) override
  {
    EXPECT_EQ(request.request(), kRequestService1);

    UServerUtils::Grpc::Test1::Response response;
    response.set_response(kResponseService1);
    call.Finish(response);
  }

private:
  const std::string name_;
};

class Test2Service final
  : public UServerUtils::Grpc::Test2::TestStreamServiceBase,
    public ReferenceCounting::AtomicImpl
{
public:
  Test2Service(const std::string& name)
    : name_(name)
  {
    EXPECT_EQ(name_, kNameService2);
  }

  ~Test2Service() override = default;

  void chat(chatCall& call) override
  {
    UServerUtils::Grpc::Test2::Request request;
    UServerUtils::Grpc::Test2::Response response;
    std::size_t count = 0;
    while (call.Read(request))
    {
      count += 1;
      response.set_allocated_response(request.release_request());
      call.Write(response);
      userver::engine::Yield();
    }
    call.Finish();
    EXPECT_EQ(count, kCountRequestService2);
  }

private:
  const std::string name_;
};

class Test1Client final
  : public UServerUtils::Component,
    public ReferenceCounting::AtomicImpl
{
public:
  Test1Client(
    const UServerUtils::UServerGrpc::GrpcClientFactory_var& factory,
    const std::string& endpoint)
    : client_(factory->make_client<UServerUtils::Grpc::Test1::TestUnaryServiceClient>(
        "Test1Client",
        endpoint))
  {
  }

  std::string say_hello(std::string&& data)
  {
    UServerUtils::Grpc::Test1::Request request;
    request.set_request(std::move(data));

    auto context = std::make_unique<grpc::ClientContext>();
    context->set_deadline(
      userver::engine::Deadline::FromDuration(std::chrono::seconds{20}));

    auto stream = client_->say_hello(request, std::move(context));
    UServerUtils::Grpc::Test1::Response response = stream.Finish();

    return std::move(*response.mutable_response());
  }

private:
  std::unique_ptr<UServerUtils::Grpc::Test1::TestUnaryServiceClient> client_;
};

class Test2Client final
  : public UServerUtils::Component,
    public ReferenceCounting::AtomicImpl
{
public:
  Test2Client(
    const UServerUtils::UServerGrpc::GrpcClientFactory_var& factory,
    const std::string& endpoint)
    : client_(factory->make_client<UServerUtils::Grpc::Test2::TestStreamServiceClient>(
        "Test2Client",
        endpoint))
  {
    EXPECT_TRUE(client_);
  }

  ~Test2Client() override = default;

  void chat()
  {
    auto context = std::make_unique<grpc::ClientContext>();
    context->AddMetadata("req_header", "value");
    context->set_deadline(
      userver::engine::Deadline::FromDuration(std::chrono::seconds{20}));
    auto call = client_->chat(std::move(context));

    UServerUtils::Grpc::Test2::Request request;
    UServerUtils::Grpc::Test2::Response response;
    for (std::size_t i = 1; i <= kCountRequestService2; ++i)
    {
      std::string data = kRequestService2 + std::to_string(i);
      request.set_request(data);
      EXPECT_TRUE(call.Write(request));
      EXPECT_TRUE(call.Read(response));
      EXPECT_EQ(response.response(), data);
      userver::engine::Yield();
    }
    EXPECT_TRUE(call.WritesDone());
  }

private:
  std::unique_ptr<UServerUtils::Grpc::Test2::TestStreamServiceClient> client_;
};

class GrpcFixture2 : public testing::Test
{
public:
  void SetUp() override
  {
    logger_ = new Logging::OStream::Logger(
      Logging::OStream::Config(
        std::cerr,
        Logging::Logger::CRITICAL));

    UServerUtils::CoroPoolConfig coro_pool_config;
    UServerUtils::EventThreadPoolConfig event_thread_pool_config;
    event_thread_pool_config.ev_default_loop_disabled = true;

    UServerUtils::TaskProcessorConfig main_task_processor_config;
    main_task_processor_config.name = "main_task_processor";
    main_task_processor_config.worker_threads = 3;
    main_task_processor_config.thread_name = "main_tskpr";

    task_processor_container_builder1_ =
      UServerUtils::TaskProcessorContainerBuilderPtr(
        new UServerUtils::TaskProcessorContainerBuilder(
          logger_.in(),
          coro_pool_config,
          event_thread_pool_config,
          main_task_processor_config));

    task_processor_container_builder2_ =
      UServerUtils::TaskProcessorContainerBuilderPtr(
        new UServerUtils::TaskProcessorContainerBuilder(
          logger_.in(),
          coro_pool_config,
          event_thread_pool_config,
          main_task_processor_config));

    UServerUtils::TaskProcessorConfig channel_task_processor_config;
    channel_task_processor_config.name = kNameChannelTaskProcessor;
    channel_task_processor_config.worker_threads = 3;
    channel_task_processor_config.thread_name = "channel_tskpr";

    task_processor_container_builder1_->add_task_processor(channel_task_processor_config);
    task_processor_container_builder2_->add_task_processor(channel_task_processor_config);
  }

  void TearDown() override
  {
  }

  UServerUtils::TaskProcessorContainerBuilderPtr task_processor_container_builder1_;
  UServerUtils::TaskProcessorContainerBuilderPtr task_processor_container_builder2_;
  Logging::Logger_var logger_;
};

} // namespace

TEST_F(GrpcFixture2, Subtest_2)
{
  auto init_func1 = [logger = logger_] (
    UServerUtils::TaskProcessorContainer& task_processor_container) {
    auto& main_task_processor =
      task_processor_container.get_main_task_processor();
    UServerUtils::ComponentsBuilderPtr components_builder =
      std::make_unique<UServerUtils::ComponentsBuilder>();
    auto& statistic_storage =
      components_builder->get_statistics_storage();

    UServerUtils::UServerGrpc::ServerConfig config_server;
    config_server.port = kPortServer1;
    UServerUtils::UServerGrpc::GrpcServerBuilderPtr server_builder =
      std::make_unique<UServerUtils::UServerGrpc::GrpcServerBuilder>(
        logger,
        std::move(config_server),
        statistic_storage);
    UServerUtils::Grpc::Server::GrpcServiceBase_var test_service(
      new Test1Service(kNameService1));
    server_builder->add_grpc_service(
      main_task_processor,
      test_service.in());
    components_builder->add_grpc_userver_server(std::move(server_builder));

    auto& channel_task_processor = task_processor_container.get_task_processor(
      kNameChannelTaskProcessor);
    UServerUtils::UServerGrpc::ClientFactoryConfig client_factory_config;
    client_factory_config.channel_count = 2;
    auto client_factory = components_builder->add_grpc_userver_client_factory(
      std::move(client_factory_config),
      channel_task_processor);
    ReferenceCounting::SmartPtr<Test1Client> test_client(
      new Test1Client(
        client_factory,
        "127.0.0.1:" + std::to_string(kPortServer1)));
    components_builder->add_user_component(
      kNameClientTest,
      test_client.in());

    return components_builder;
  };

  auto init_func2 = [logger = logger_] (
    UServerUtils::TaskProcessorContainer& task_processor_container) {
    auto& main_task_processor =
      task_processor_container.get_main_task_processor();

    UServerUtils::ComponentsBuilderPtr components_builder =
      std::make_unique<UServerUtils::ComponentsBuilder>();
    auto& statistic_storage =
      components_builder->get_statistics_storage();

    UServerUtils::UServerGrpc::ServerConfig config_server;
    config_server.port = kPortServer2;
    UServerUtils::UServerGrpc::GrpcServerBuilderPtr server_builder =
      std::make_unique<UServerUtils::UServerGrpc::GrpcServerBuilder>(
        logger,
        std::move(config_server),
        statistic_storage);
    UServerUtils::Grpc::Server::GrpcServiceBase_var test_service(
      new Test2Service(kNameService2));
    server_builder->add_grpc_service(
      main_task_processor,
      test_service.in());
    components_builder->add_grpc_userver_server(std::move(server_builder));

    auto& channel_task_processor = task_processor_container.get_task_processor(
      kNameChannelTaskProcessor);
    UServerUtils::UServerGrpc::ClientFactoryConfig client_factory_config;
    client_factory_config.channel_count = 2;
    auto client_factory = components_builder->add_grpc_userver_client_factory(
      std::move(client_factory_config),
      channel_task_processor);
    ReferenceCounting::SmartPtr<Test2Client> test_client(
      new Test2Client(
        client_factory,
        "127.0.0.1:" + std::to_string(kPortServer2)));
    components_builder->add_user_component(
      kNameClientTest,
      test_client.in());

    return components_builder;
  };

  UServerUtils::Manager_var manager1 = new UServerUtils::Manager(
    std::move(task_processor_container_builder1_),
    std::move(init_func1),
    logger_.in());

  UServerUtils::Manager_var manager2 = new UServerUtils::Manager(
    std::move(task_processor_container_builder2_),
    std::move(init_func2),
    logger_.in());

  manager1->activate_object();
  manager2->activate_object();

  auto& client1 = manager1->get_user_component<Test1Client>(kNameClientTest);
  auto response_service = UServerUtils::Utils::run_in_coro(
    manager1->get_main_task_processor(),
    UServerUtils::Utils::Importance::kCritical,
    {},
    [&client1] () {
      std::string message_service1 = kRequestService1;
      return client1.say_hello(std::move(message_service1));
  });
  EXPECT_EQ(response_service, kResponseService1);

  auto& client2 = manager2->get_user_component<Test2Client>(kNameClientTest);
  UServerUtils::Utils::run_in_coro(
    manager2->get_main_task_processor(),
    UServerUtils::Utils::Importance::kCritical,
    {},
    [&client2] () {
      client2.chat();
  });

  manager1->deactivate_object();
  manager2->deactivate_object();

  manager1->wait_object();
  manager2->wait_object();

  EXPECT_TRUE(kCheckService1Activate);
  EXPECT_TRUE(kCheckService1Deactivate);
}