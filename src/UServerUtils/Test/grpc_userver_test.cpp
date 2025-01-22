// STD
#include <chrono>
#include <iostream>

// GTEST
#include <gtest/gtest.h>

// USERVER
#include <userver/engine/sleep.hpp>
#include <userver/ugrpc/client/impl/completion_queue_pool.hpp>

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

const std::string kNameClientTest1 = "client1";
const std::string kNameClientTest2 = "client2";

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

  say_helloResult say_hello(
    CallContext& context,
    ::UServerUtils::Grpc::Test1::Request&& request) override
  {
    EXPECT_EQ(request.request(), kRequestService1);
    UServerUtils::Grpc::Test1::Response response;
    response.set_response(kResponseService1);
    return response;
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

  chatResult chat(CallContext& context, chatReaderWriter& stream) override
  {
    UServerUtils::Grpc::Test2::Request request;
    UServerUtils::Grpc::Test2::Response response;
    std::size_t count = 0;
    while (stream.Read(request))
    {
      count += 1;
      response.set_allocated_response(request.release_request());
      stream.Write(response);
    }

    EXPECT_EQ(count, kCountRequestService2);
    return grpc::Status::OK;
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
    UServerUtils::UServerGrpc::GrpcClientFactory* factory,
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
    auto response = client_->say_hello(request, std::move(context));

    return std::move(*response.mutable_response());
  }

protected:
  ~Test1Client() = default;

private:
  std::unique_ptr<UServerUtils::Grpc::Test1::TestUnaryServiceClient> client_;
};

class Test2Client final
  : public UServerUtils::Component,
    public ReferenceCounting::AtomicImpl
{
public:
  Test2Client(
    UServerUtils::UServerGrpc::GrpcClientFactory* factory,
    const std::string& endpoint)
    : client_(factory->make_client<UServerUtils::Grpc::Test2::TestStreamServiceClient>(
        "Test2Client",
        endpoint))
  {
    EXPECT_TRUE(client_);
  }

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
    }
    EXPECT_TRUE(call.WritesDone());
  }

protected:
  ~Test2Client() override = default;

private:
  std::unique_ptr<UServerUtils::Grpc::Test2::TestStreamServiceClient> client_;
};

class GrpcFixture : public testing::Test
{
public:
  using CompletionQueuePool = userver::ugrpc::client::impl::CompletionQueuePool;

public:
  void SetUp() override
  {
    logger_ = new Logging::OStream::Logger(
      Logging::OStream::Config(
        std::cerr,
        Logging::Logger::CRITICAL));

    UServerUtils::CoroPoolConfig coro_pool_config;
    UServerUtils::EventThreadPoolConfig event_thread_pool_config;

    UServerUtils::TaskProcessorConfig main_task_processor_config;
    main_task_processor_config.name = "main_task_processor";
    main_task_processor_config.worker_threads = 3;
    main_task_processor_config.thread_name = "main_tskpr";

    task_processor_container_builder_ =
      std::make_unique<UServerUtils::TaskProcessorContainerBuilder>(
        logger_.in(),
        coro_pool_config,
        event_thread_pool_config,
        main_task_processor_config);

    UServerUtils::TaskProcessorConfig channel_task_processor_config;
    channel_task_processor_config.name = kNameChannelTaskProcessor;
    channel_task_processor_config.worker_threads = 3;
    channel_task_processor_config.thread_name = "channel_tskpr";

    task_processor_container_builder_->add_task_processor(
      channel_task_processor_config);
  }

  void TearDown() override
  {
  }

  UServerUtils::TaskProcessorContainerBuilderPtr task_processor_container_builder_;
  Logging::Logger_var logger_;
};

} // namespace

TEST_F(GrpcFixture, test)
{
  auto init_func = [logger = logger_] (
    UServerUtils::TaskProcessorContainer& task_processor_container) {
    auto& main_task_processor =
      task_processor_container.get_main_task_processor();

    auto components_builder =
      std::make_unique<UServerUtils::ComponentsBuilder>();
    auto& statistic_storage =
      components_builder->get_statistics_storage();

    UServerUtils::UServerGrpc::ServerConfig config_server1;
    config_server1.port = kPortServer1;
    UServerUtils::UServerGrpc::GrpcServerBuilderPtr server_builder1 =
      std::make_unique<UServerUtils::UServerGrpc::GrpcServerBuilder>(
        logger.in(),
        std::move(config_server1),
        statistic_storage);
    UServerUtils::Grpc::Server::GrpcServiceBase_var test1_service(
      new Test1Service(kNameService1));
    server_builder1->add_grpc_service(
      main_task_processor,
      test1_service.in());
    components_builder->add_grpc_userver_server(std::move(server_builder1));

    UServerUtils::UServerGrpc::ServerConfig config_server2;
    config_server2.port = kPortServer2;
    UServerUtils::UServerGrpc::GrpcServerBuilderPtr server_builder2 =
      std::make_unique<UServerUtils::UServerGrpc::GrpcServerBuilder>(
        logger.in(),
        std::move(config_server2),
        statistic_storage);
    UServerUtils::Grpc::Server::GrpcServiceBase_var test2_service(
      new Test2Service(kNameService2));
    server_builder2->add_grpc_service(
      main_task_processor,
      test2_service.in());
    components_builder->add_grpc_userver_server(std::move(server_builder2));

    auto& channel_task_processor =
      task_processor_container.get_task_processor(
        kNameChannelTaskProcessor);
    UServerUtils::UServerGrpc::ClientFactoryConfig client_factory_config;
    client_factory_config.channel_count = 2;
    auto queue_pool = std::make_shared<CompletionQueuePool>(5);
    auto client_factory = components_builder->add_grpc_userver_client_factory(
      std::move(client_factory_config),
      channel_task_processor,
      queue_pool);
    ReferenceCounting::SmartPtr<Test1Client> test1_client(
      new Test1Client(
        client_factory.in(),
        "127.0.0.1:" + std::to_string(kPortServer1)));
    components_builder->add_user_component(
      kNameClientTest1,
      test1_client.in());
    ReferenceCounting::SmartPtr<Test2Client> test2_client(
      new Test2Client(
        client_factory.in(),
        "127.0.0.1:" + std::to_string(kPortServer2)));
    components_builder->add_user_component(
      kNameClientTest2,
      test2_client.in());

    return components_builder;
  };

  UServerUtils::Manager_var manager = new UServerUtils::Manager(
    std::move(task_processor_container_builder_),
    std::move(init_func),
    logger_.in());

  manager->activate_object();

  auto& client1 = manager->get_user_component<Test1Client>(kNameClientTest1);
  auto response_service1 = UServerUtils::Utils::run_in_coro(
    manager->get_main_task_processor(),
    UServerUtils::Utils::Importance::kCritical,
    {},
    [&client1] () {
      std::string message_service1 = kRequestService1;
      return client1.say_hello(std::move(message_service1));
  });
  EXPECT_EQ(response_service1, kResponseService1);

  auto& client2 = manager->get_user_component<Test2Client>(kNameClientTest2);
  UServerUtils::Utils::run_in_coro(
    manager->get_main_task_processor(),
    UServerUtils::Utils::Importance::kCritical,
    {},
    [&client2] () {
      client2.chat();
  });

  manager->deactivate_object();
  manager->wait_object();

  EXPECT_TRUE(kCheckService1Activate);
  EXPECT_TRUE(kCheckService1Deactivate);
}