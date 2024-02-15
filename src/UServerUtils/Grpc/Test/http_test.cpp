// GTEST
#include <gtest/gtest.h>

// STD
#include <thread>

// USERVER
#include <userver/dynamic_config/value.hpp>

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
const std::size_t kMonitorPort = 7778;
const std::string kKeyParam = "key";
const std::string kValueParam = "value";
const std::string kResponseData = "response";
const std::string kPathService = "/test_service";
const std::string kNameHeader = "header";
const std::string kValueHeader = "value_header";
const userver::server::http::HttpStatus kServerStatusCode =
  userver::server::http::HttpStatus::kConflict;
const userver::clients::http::Status kClientStatusCode =
  userver::clients::http::Status::Conflict;
const std::string kStatisticsPrefix = "http_cobrazz_prefix_test";
const std::size_t kStatisticsValue = 777;
const std::map<std::string, std::string> kStatisticsLabels = {
  {"label_1", "value_1"},
  {"label_2", "value_2"},
  {"label_3", "value_3"}
};

std::atomic<bool> kIsCallStreamHandler{false};
std::atomic<std::size_t> kCountGet{0};
std::atomic<std::size_t> kCountPost{0};

} // namespace

class TestHandler final
  : public HttpHandler,
    public ReferenceCounting::AtomicImpl
{
public:
  using StatisticsHolder = userver::utils::statistics::Entry;
  using StatisticsHolderPtr = std::shared_ptr<StatisticsHolder>;

public:
  TestHandler(
    const StatisticsHolderPtr& statistics_holder,
    const std::string& handler_name,
    const HandlerConfig& handler_config,
    const std::optional<Level> log_level = {})
    : HttpHandler(handler_name, handler_config, log_level),
      statistics_holder_(statistics_holder)
  {
  }

  ~TestHandler() override = default;

  std::string handle_request_throw(
    const HttpRequest& request,
    RequestContext& context) const override
  {
    const auto& method = request.GetMethod();
    if (method == userver::server::http::HttpMethod::kGet)
    {
      kCountGet.fetch_add(1);
    }
    else if (method == userver::server::http::HttpMethod::kPost)
    {
      kCountPost.fetch_add(1);
    }

    auto& response = request.GetHttpResponse();
    response.SetHeader(kNameHeader, kValueHeader);
    return kResponseData + request.GetArg(kKeyParam);
  }

  void handle_stream_request(
    const HttpRequest& request,
    RequestContext& context,
    ResponseBodyStream& response_body_stream) const override
  {
    kIsCallStreamHandler = true;

    const auto& method = request.GetMethod();
    if (method == userver::server::http::HttpMethod::kGet)
    {
      kCountGet.fetch_add(1);
    }
    else if (method == userver::server::http::HttpMethod::kPost)
    {
      kCountPost.fetch_add(1);
    }

    response_body_stream.SetStatusCode(kServerStatusCode);
    response_body_stream.SetHeader(kNameHeader, kValueHeader);
    const std::string data = kResponseData + request.GetArg(kKeyParam);

    for (const char ch : data)
    {
      response_body_stream.PushBodyChunk(
        std::string(1, ch),
        {});
    }
  }

private:
  const StatisticsHolderPtr statistics_holder_;
};

class HttpFixture : public testing::Test
{
public:
  enum class Method
  {
    GET,
    POST
  };

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

  void run(
    const std::optional<std::string> unix_socket_path,
    const bool response_body_stream,
    const Method method)
  {
    kIsCallStreamHandler = false;
    kCountGet = 0;
    kCountPost = 0;

    auto init_func = [logger = logger_, unix_socket_path, response_body_stream] (
      TaskProcessorContainer& task_processor_container) {

      auto& main_task_processor = task_processor_container.get_main_task_processor();
      auto components_builder = std::make_unique<ComponentsBuilder>();

      auto& statistic_storage = components_builder->get_statistics_storage();
      auto entry = statistic_storage.RegisterWriter(
        kStatisticsPrefix,
        [] (userver::utils::statistics::Writer& writer) {
          std::vector<userver::utils::statistics::LabelView> labels;
          labels.reserve(kStatisticsLabels.size());
          for (const auto& [name, value] : kStatisticsLabels)
          {
            labels.emplace_back(name, value);
          }

          writer.ValueWithLabels(
            kStatisticsValue,
            userver::utils::statistics::LabelsSpan(labels));
        },
        {});
      auto statistics_holder =
        std::make_shared<userver::utils::statistics::Entry>(
          std::move(entry));

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

      ListenerConfig monitor_listener_config;
      monitor_listener_config.port = kMonitorPort;
      server_config.monitor_listener_config = monitor_listener_config;

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
      handler_config.response_body_stream = response_body_stream;
      ReferenceCounting::SmartPtr<TestHandler> test_handler_get(
        new TestHandler(
          statistics_holder,
          "TestHandlerGet",
          handler_config));
      http_server_builder->add_handler(
        test_handler_get.in(),
        main_task_processor);

      handler_config.method = "POST";
      ReferenceCounting::SmartPtr<TestHandler> test_handler_post(
        new TestHandler(
          statistics_holder,
          "TestHandlerPost",
          handler_config));
      http_server_builder->add_handler(
        test_handler_post.in(),
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

    auto request = client.create_request().retry(10).timeout(1000);
    if (method == Method::GET)
    {
      request.method(HttpMethod::kGet);
    }
    else if (method == Method::POST)
    {
      request.method(HttpMethod::kPost);
    }

    const std::size_t number_iterations = 1000;
    for (std::size_t i = 1; i <= number_iterations; ++i)
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

      if (response_body_stream)
      {
        EXPECT_EQ(response->status_code(), kClientStatusCode);
        EXPECT_EQ(kIsCallStreamHandler, true);
      }
      else
      {
        EXPECT_TRUE(response->IsOk());
        EXPECT_EQ(kIsCallStreamHandler, false);
      }
      EXPECT_EQ(response->body(), kResponseData + std::to_string(i));
      EXPECT_EQ(response->headers()[kNameHeader], kValueHeader);
    }

    if (method == Method::GET)
    {
      EXPECT_EQ(kCountGet, number_iterations);
    }
    else if (method == Method::POST)
    {
      EXPECT_EQ(kCountPost, number_iterations);
    }

    auto request_monitor = client.create_request()
      .retry(10)
      .timeout(1000)
      .method(HttpMethod::kGet)
      .url("http://127.0.0.1:" + std::to_string(kMonitorPort) + "/metrics?format=json");
    auto response_monotor = request_monitor.perform();
    EXPECT_TRUE(response_monotor->IsOk());

    userver::dynamic_config::DocsMap docs_map;
    docs_map.Parse(response_monotor->body(), true);
    userver::formats::json::Value array = docs_map.Get(kStatisticsPrefix);
    EXPECT_TRUE(array.IsArray());
    if (array.IsArray())
    {
      EXPECT_EQ(array.GetSize(), 1);
      if (array.GetSize() == 1)
      {
        const auto value = array[0];
        EXPECT_TRUE(value["value"].IsInt());
        if (value["value"].IsInt())
        {
          EXPECT_EQ(value["value"].As<int>(), kStatisticsValue);
        }

        EXPECT_TRUE(value["labels"].IsObject());
        if (value["labels"].IsObject())
        {
          auto labels = value["labels"];
          std::map<std::string, std::string> result_labels;
          for (const auto& [k, v]: userver::formats::common::Items(labels))
          {
            if (v.IsString())
            {
              result_labels.try_emplace(k, v.As<std::string>());
            }
          }

          EXPECT_EQ(result_labels, kStatisticsLabels);
        }
      }
    }

    manager->deactivate_object();
    manager->wait_object();
  }

  TaskProcessorContainerBuilderPtr task_processor_container_builder_;
  Logging::Logger_var logger_;
};

TEST_F(HttpFixture, TCP_GET)
{
  run({}, false, Method::GET);
}

TEST_F(HttpFixture, TCP_POST)
{
  run({}, false, Method::POST);
}

TEST_F(HttpFixture, TCP_STREAM_BODY_GET)
{
  run({}, true, Method::GET);
}

TEST_F(HttpFixture, TCP_STREAM_BODY_POST)
{
  run({}, true, Method::POST);
}

TEST_F(HttpFixture, UNIX_SOCKET_GET)
{
  run("/tmp/service.socket", false, Method::GET);
}

TEST_F(HttpFixture, UNIX_SOCKET_POST)
{
  run("/tmp/service.socket", false, Method::POST);
}

TEST_F(HttpFixture, UNIX_SOCKET_STREAM_BODY_GET)
{
  run("/tmp/service.socket", true, Method::GET);
}

TEST_F(HttpFixture, UNIX_SOCKET_STREAM_BODY_POST)
{
  run("/tmp/service.socket", true, Method::POST);
}