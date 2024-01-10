// GTEST
#include <gtest/gtest.h>

// STD
#include <thread>

// THIS
#include <Logger/Logger.hpp>
#include <Logger/StreamLogger.hpp>
#include <UServerUtils/Grpc/Http/Client/Client.hpp>
#include <UServerUtils/Grpc/ComponentsBuilder.hpp>
#include <UServerUtils/Grpc/Manager.hpp>
#include <UServerUtils/Grpc/Utils.hpp>

using namespace UServerUtils::Grpc;
using namespace UServerUtils::Http::Client;
using namespace UServerUtils::Http::Server;

namespace
{

const std::size_t kPort = 7777;
const std::string kKeyParam = "key";
const std::string kValueParam = "value";
const std::string kResponseData = "response";
const std::string kPathService = "/test_service";
const std::string kNameHeader = "header";
const std::string kValueHeader = "value_header";

} // namespace

class TestHandler final
  : public HttpHandler,
    public ReferenceCounting::AtomicImpl
{
public:
  TestHandler(
    const std::string& handler_name,
    const HandlerConfig& handler_config,
    const std::optional<Level> log_level = {})
    : HttpHandler(
        handler_name,
        handler_config,
        log_level)
  {
  }

  ~TestHandler() override = default;

  std::string handle_request_throw(
    const HttpRequest& request,
    RequestContext& context) const override
  {
    auto& response = request.GetHttpResponse();
    response.SetHeader(kNameHeader, kValueHeader);
    return kResponseData + request.GetArg(kKeyParam);
  }
};

class HttpFixture : public testing::Test
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

    task_processor_container_builder_ =
      std::make_unique<TaskProcessorContainerBuilder>(
        logger_.in(),
        coro_pool_config,
        event_thread_pool_config,
        main_task_processor_config);
  }

  void TearDown() override
  {
  }

  void run(const std::optional<std::string> unix_socket_path)
  {
    auto init_func = [logger = logger_, unix_socket_path] (TaskProcessorContainer& task_processor_container) {
      auto& main_task_processor = task_processor_container.get_main_task_processor();
      auto components_builder = std::make_unique<ComponentsBuilder>();
      auto& statistic_storage = components_builder->get_statistics_storage();

      ServerConfig server_config;
      server_config.server_name = "TestServer";

      auto& listener_config = server_config.listener_config;
      listener_config.max_connections = 300000;
      if (unix_socket_path.has_value())
      {
        listener_config.unix_socket_path = *unix_socket_path;
      }
      else
      {
        listener_config.port = kPort;
      }

      auto& connection_config = listener_config.connection_config;
      connection_config.keepalive_timeout = std::chrono::seconds{1000000};
      connection_config.requests_queue_size_threshold = 100000;

      listener_config.handler_defaults = {};

      auto http_server_builder = std::make_unique<HttpServerBuilder>(
        logger.in(),
        server_config,
        main_task_processor,
        statistic_storage);

      HandlerConfig handler_config;
      handler_config.method = "GET";
      handler_config.path = kPathService;
      ReferenceCounting::SmartPtr<TestHandler> test_handler(
        new TestHandler(
          "TestHandler",
          handler_config));

      http_server_builder->add_handler(
        test_handler.in(),
        main_task_processor);

      components_builder->add_http_server(std::move(http_server_builder));

      return components_builder;
    };

    Manager_var manager(
      new Manager(
        std::move(task_processor_container_builder_),
        std::move(init_func),
        logger_.in()));

    manager->activate_object();

    auto& task_processor = manager->get_main_task_processor();
    ClientConfig client_config;
    client_config.io_threads = 8;
    Client client(client_config, task_processor);

    auto request =
      client.create_not_signed_request()
        .method(HttpMethod::kGet)
        .retry(10)
        .timeout(1000);

    for (std::size_t i = 1; i <= 1; ++i)
    {
      if (unix_socket_path.has_value())
      {
        request.unix_socket_path(*unix_socket_path);
        const std::string url = "http://localhost/" + kPathService
          + "?" + kKeyParam + "=" + std::to_string(i);
        request.url(url);
      }
      else
      {
        const std::string url = "http://127.0.0.1:" + std::to_string(kPort) +
          kPathService + "?" + kKeyParam + "=" + std::to_string(i);
        request.url(url);
      }

      auto response = request.perform();
      EXPECT_TRUE(response->IsOk());
      EXPECT_EQ(response->body(), kResponseData + std::to_string(i));
      EXPECT_EQ(response->headers()[kNameHeader], kValueHeader);
    }

    manager->deactivate_object();
    manager->wait_object();
  }

  TaskProcessorContainerBuilderPtr task_processor_container_builder_;
  Logging::Logger_var logger_;
};

TEST_F(HttpFixture, TCP)
{
  run({});
}

TEST_F(HttpFixture, UNIX_SOCKET)
{
  run("/tmp/service.socket");
}