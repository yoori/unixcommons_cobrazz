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
#include <UServerUtils/Grpc/Statistics/CommonStatisticsProvider.hpp>
#include <UServerUtils/Grpc/Statistics/CompositeStatisticsProvider.hpp>
#include <UServerUtils/Grpc/Statistics/CounterStatisticsProvider.hpp>
#include <UServerUtils/Grpc/Statistics/TimeStatisticsProvider.hpp>

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

enum class TimeEnumId
{
  Test1,
  Test2,
  Max
};

enum class CounterEnumId
{
  Test1,
  Test2,
  Test3,
  Test4,
  Max
};

enum class CommonEnumId
{
  Test1,
  Test2,
  Max
};

class EnumToStringConverter
{
public:
  auto operator()()
  {
    const std::map<TimeEnumId, std::string> id_to_name = {
      {TimeEnumId::Test1, "Test1"},
      {TimeEnumId::Test2, "Test2"}
    };

    return id_to_name;
  }
};

class EnumCounterConverter
{
public:
  auto operator()()
  {
    const std::map<CounterEnumId, std::pair<UServerUtils::Statistics::CounterType, std::string>> id_to_name = {
      {CounterEnumId::Test1, {UServerUtils::Statistics::CounterType::UInt, "Test1"}},
      {CounterEnumId::Test2, {UServerUtils::Statistics::CounterType::Int, "Test2"}},
      {CounterEnumId::Test3, {UServerUtils::Statistics::CounterType::Double, "Test3"}},
      {CounterEnumId::Test4, {UServerUtils::Statistics::CounterType::Bool, "Test4"}},
    };

    return id_to_name;
  }
};

class EnumCommonConverter
{
public:
  auto operator()()
  {
    const std::map<CommonEnumId, std::pair<UServerUtils::Statistics::CommonType, std::string>> id_to_name = {
      {CommonEnumId::Test1, {UServerUtils::Statistics::CommonType::UInt, "Test1"}},
      {CommonEnumId::Test2, {UServerUtils::Statistics::CommonType::Double, "Test2"}}
    };

    return id_to_name;
  }
};

[[maybe_unused]] auto get_time_statistics_provider_test()
{
  return UServerUtils::Statistics::get_time_statistics_provider<
    TimeEnumId,
    EnumToStringConverter,
    4,
    50>();
}

[[maybe_unused]] auto get_counter_statistics_provider_test()
{
  return UServerUtils::Statistics::get_counter_statistics_provider<
    CounterEnumId,
    EnumCounterConverter>();
}

[[maybe_unused]] auto get_common_statistics_provider_test()
{
  return UServerUtils::Statistics::get_common_statistics_provider<
    CommonEnumId,
    EnumCommonConverter,
    std::shared_mutex,
    std::map>();
}

} // namespace

#define DO_TIME_STATISTIC(id) \
  auto measure = get_time_statistics_provider_test()->make_measure(id);

#define ADD_COUNTER_STATISTIC(id, value) \
  get_counter_statistics_provider_test()->add(id, value);

#define ADD_COMMON_STATISTIC(id, label, value) \
  get_common_statistics_provider_test()->add(id, label, value);


class TestStatisticsProvider final : public UServerUtils::Statistics::StatisticsProvider
{
public:
  TestStatisticsProvider() = default;

  ~TestStatisticsProvider() override = default;

  std::string name() override
  {
    return "test_statistic";
  }

private:
  void write(UServerUtils::Statistics::Writer& writer) override
  {
    std::vector<userver::utils::statistics::LabelView> labels;
    labels.reserve(kStatisticsLabels.size());
    for (const auto& [name, value] : kStatisticsLabels)
    {
      labels.emplace_back(name, value);
    }

    writer.ValueWithLabels(
      kStatisticsValue,
      userver::utils::statistics::LabelsSpan(labels));
  }
};

class TestHandler final
  : public HttpHandler,
    public ReferenceCounting::AtomicImpl
{
public:
  using StatisticsHolder = userver::utils::statistics::Entry;
  using StatisticsHolderPtr = std::shared_ptr<StatisticsHolder>;

public:
  TestHandler(
    const std::string& handler_name,
    const HandlerConfig& handler_config,
    const std::optional<Level> log_level = {})
    : HttpHandler(handler_name, handler_config, log_level)
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
    const Method method,
    const bool enable_statistics_test)
  {
    kIsCallStreamHandler = false;
    kCountGet = 0;
    kCountPost = 0;

    auto test_statistics_provider = std::make_shared<TestStatisticsProvider>();
    auto time_statistics_provider = get_time_statistics_provider_test();
    auto counter_statistics_provider = get_counter_statistics_provider_test();
    auto common_statistics_provider = get_common_statistics_provider_test();

    auto statistics_provider = std::make_shared<
      UServerUtils::Statistics::CompositeStatisticsProviderImpl<std::shared_mutex>>(
        logger_.in());
    statistics_provider->add(test_statistics_provider);
    statistics_provider->add(time_statistics_provider);
    statistics_provider->add(counter_statistics_provider);
    statistics_provider->add(common_statistics_provider);

    auto init_func = [logger = logger_, unix_socket_path, response_body_stream, statistics_provider] (
      TaskProcessorContainer& task_processor_container) {

      auto& main_task_processor = task_processor_container.get_main_task_processor();

      ComponentsBuilder::StatisticsProviderInfo statistics_provider_info;
      statistics_provider_info.statistics_provider = statistics_provider;
      statistics_provider_info.statistics_prefix = kStatisticsPrefix;
      auto components_builder = std::make_unique<ComponentsBuilder>(std::move(statistics_provider_info));
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
          "TestHandlerGet",
          handler_config));
      http_server_builder->add_handler(
        test_handler_get.in(),
        main_task_processor);

      handler_config.method = "POST";
      ReferenceCounting::SmartPtr<TestHandler> test_handler_post(
        new TestHandler(
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

    if (enable_statistics_test)
    {
      do_statistic_test(client, test_statistics_provider);
    }

    manager->deactivate_object();
    manager->wait_object();
  }

  void do_statistic_test(
    Client& client,
    const std::shared_ptr<TestStatisticsProvider>& test_statistics_provider)
  {
    time_statistic_test();
    counter_statistic_test();
    common_statistic_test();

    auto request_monitor = client.create_request()
      .retry(10)
      .timeout(1000)
      .method(HttpMethod::kGet)
      .url("http://127.0.0.1:" + std::to_string(kMonitorPort) + "/metrics?format=json");
    auto response_monotor = request_monitor.perform();
    EXPECT_TRUE(response_monotor->IsOk());

    const auto body = response_monotor->body();
    const auto test_name = test_statistics_provider->name();
    check_test_statistic(body, test_name);
    const auto time_name = get_time_statistics_provider_test()->name();
    const auto counter_name = get_counter_statistics_provider_test()->name();
    const auto common_name = get_common_statistics_provider_test()->name();
    check_time_statistic(body, time_name);
    check_counter_statistic(body, counter_name);
    check_common_statistic(body, common_name);
  }

  void check_test_statistic(const std::string& data, const std::string& name)
  {
    userver::dynamic_config::DocsMap docs_map;
    docs_map.Parse(data, true);
    userver::formats::json::Value array = docs_map.Get(kStatisticsPrefix + "." + name);
    EXPECT_TRUE(array.IsArray());
    if (array.IsArray())
    {
      EXPECT_EQ(array.GetSize(), 1);
      if (array.GetSize() == 1)
      {
        const auto value = array[0];
        EXPECT_TRUE(value["value"].IsInt());
        EXPECT_EQ(value["value"].As<int>(), kStatisticsValue);
        EXPECT_TRUE(value["labels"].IsObject());

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

  void time_statistic_test()
  {
    for (std::size_t  i = 1; i <= 1; ++i)
    {
      DO_TIME_STATISTIC(TimeEnumId::Test1)
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    for (std::size_t  i = 1; i <= 2; ++i)
    {
      DO_TIME_STATISTIC(TimeEnumId::Test1)
      std::this_thread::sleep_for(std::chrono::milliseconds(55));
    }

    for (std::size_t  i = 1; i <= 3; ++i)
    {
      DO_TIME_STATISTIC(TimeEnumId::Test1)
      std::this_thread::sleep_for(std::chrono::milliseconds(105));
    }

    for (std::size_t  i = 1; i <= 4; ++i)
    {
      DO_TIME_STATISTIC(TimeEnumId::Test1)
      std::this_thread::sleep_for(std::chrono::milliseconds(155));
    }

    for (std::size_t  i = 1; i <= 5; ++i)
    {
      DO_TIME_STATISTIC(TimeEnumId::Test2)
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }

  void check_time_statistic(const std::string& data, const std::string& name)
  {
    userver::dynamic_config::DocsMap docs_map;
    docs_map.Parse(data, true);
    userver::formats::json::Value array = docs_map.Get(kStatisticsPrefix + "." + name);
    EXPECT_TRUE(array.IsArray());
    if (array.IsArray())
    {
      std::list<int> value_list;
      std::list<std::string> name_list;
      EXPECT_EQ(array.GetSize(), 8);
      if (array.GetSize() == 8)
      {
        value_list.emplace_back(array[0]["value"].As<int>());
        value_list.emplace_back(array[1]["value"].As<int>());
        value_list.emplace_back(array[2]["value"].As<int>());
        value_list.emplace_back(array[3]["value"].As<int>());
        value_list.emplace_back(array[4]["value"].As<int>());
        value_list.emplace_back(array[5]["value"].As<int>());
        value_list.emplace_back(array[6]["value"].As<int>());
        value_list.emplace_back(array[7]["value"].As<int>());
        value_list.sort();

        EXPECT_EQ(value_list, std::list<int>({0, 0, 0, 1, 2, 3, 4, 5}));

        name_list.emplace_back(array[0]["labels"]["name"].As<std::string>());
        name_list.emplace_back(array[1]["labels"]["name"].As<std::string>());
        name_list.emplace_back(array[2]["labels"]["name"].As<std::string>());
        name_list.emplace_back(array[3]["labels"]["name"].As<std::string>());
        name_list.emplace_back(array[4]["labels"]["name"].As<std::string>());
        name_list.emplace_back(array[5]["labels"]["name"].As<std::string>());
        name_list.emplace_back(array[6]["labels"]["name"].As<std::string>());
        name_list.emplace_back(array[7]["labels"]["name"].As<std::string>());
        name_list.sort();

        EXPECT_EQ(name_list, std::list<std::string>({"Test1", "Test1", "Test1", "Test1", "Test2", "Test2", "Test2", "Test2"}));
      }
    }
  }

  void counter_statistic_test()
  {
    for (std::size_t  i = 1; i <= 1; ++i)
    {
      ADD_COUNTER_STATISTIC(CounterEnumId::Test1, 1)
    }

    for (std::size_t  i = 1; i <= 2; ++i)
    {
      ADD_COUNTER_STATISTIC(CounterEnumId::Test2, 1)
    }

    for (std::size_t  i = 1; i <= 3; ++i)
    {
      ADD_COUNTER_STATISTIC(CounterEnumId::Test3, 1.5)
    }

    for (std::size_t  i = 1; i <= 4; ++i)
    {
      ADD_COUNTER_STATISTIC(CounterEnumId::Test4, true)
    }
  }

  void check_counter_statistic(const std::string& data, const std::string& name)
  {
    userver::dynamic_config::DocsMap docs_map;
    docs_map.Parse(data, true);
    userver::formats::json::Value array = docs_map.Get(kStatisticsPrefix + "." + name);
    EXPECT_TRUE(array.IsArray());
    if (array.IsArray())
    {
      std::list<std::string> value_list;
      std::list<std::string> name_list;
      EXPECT_EQ(array.GetSize(), 4);
      if (array.GetSize() == 4)
      {
        for (std::size_t i = 0; i < 4; ++i)
        {
          auto value = array[i]["value"];
          if (value.IsInt())
          {
            value_list.emplace_back(std::to_string(value.As<int>()));
          }
          else if (value.IsDouble())
          {
            std::ostringstream stream;
            stream << std::fixed
                   << std::setprecision(1)
                   << value.As<double>();
             value_list.emplace_back(stream.str());
          }
        }
        value_list.sort();

        EXPECT_EQ(value_list, std::list<std::string>({"1", "1", "2", "4.5"}));

        name_list.emplace_back(array[0]["labels"]["name"].As<std::string>());
        name_list.emplace_back(array[1]["labels"]["name"].As<std::string>());
        name_list.emplace_back(array[2]["labels"]["name"].As<std::string>());
        name_list.emplace_back(array[3]["labels"]["name"].As<std::string>());
        name_list.sort();

        EXPECT_EQ(name_list, std::list<std::string>({"Test1", "Test2", "Test3", "Test4"}));
      }
    }
  }

  void common_statistic_test()
  {
    for (std::size_t  i = 1; i <= 3; ++i)
    {
      ADD_COMMON_STATISTIC(CommonEnumId::Test1, std::string_view("label1"), 1)
    }

    for (std::size_t  i = 1; i <= 3; ++i)
    {
      ADD_COMMON_STATISTIC(CommonEnumId::Test2, std::string_view("label2"), 1.5)
    }

    for (std::size_t  i = 1; i <= 4; ++i)
    {
      ADD_COMMON_STATISTIC(CommonEnumId::Test1, 777, 1)
    }
  }

  void check_common_statistic(const std::string& data, const std::string& name)
  {
    userver::dynamic_config::DocsMap docs_map;
    docs_map.Parse(data, true);
    userver::formats::json::Value array = docs_map.Get(kStatisticsPrefix + "." + name);
    EXPECT_TRUE(array.IsArray());
    if (array.IsArray())
    {
      std::list<std::string> value_list;
      std::list<std::string> name_list;
      EXPECT_EQ(array.GetSize(), 3);
      if (array.GetSize() == 3)
      {
        for (std::size_t i = 0; i < 3; ++i)
        {
          auto value = array[i]["value"];
          if (value.IsInt())
          {
            value_list.emplace_back(std::to_string(value.As<int>()));
          }
          else if (value.IsDouble())
          {
            std::ostringstream stream;
            stream << std::fixed
                   << std::setprecision(1)
                   << value.As<double>();
            value_list.emplace_back(stream.str());
          }
        }
        value_list.sort();

        EXPECT_EQ(value_list, std::list<std::string>({"3", "4", "4.5"}));
      }
    }
  }

  TaskProcessorContainerBuilderPtr task_processor_container_builder_;
  Logging::Logger_var logger_;
};

TEST_F(HttpFixture, TCP_GET)
{
  run({}, false, Method::GET, false);
}

TEST_F(HttpFixture, TCP_POST)
{
  run({}, false, Method::POST, false);
}

TEST_F(HttpFixture, TCP_STREAM_BODY_GET)
{
  run({}, true, Method::GET, false);
}

TEST_F(HttpFixture, TCP_STREAM_BODY_POST)
{
  run({}, true, Method::POST, false);
}

TEST_F(HttpFixture, UNIX_SOCKET_GET)
{
  run("/tmp/service.socket", false, Method::GET, false);
}

TEST_F(HttpFixture, UNIX_SOCKET_POST)
{
  run("/tmp/service.socket", false, Method::POST, false);
}

TEST_F(HttpFixture, UNIX_SOCKET_STREAM_BODY_GET)
{
  run("/tmp/service.socket", true, Method::GET, false);
}

TEST_F(HttpFixture, UNIX_SOCKET_STREAM_BODY_POST)
{
  run("/tmp/service.socket", true, Method::POST, false);
}

TEST_F(HttpFixture, STATISTICS)
{
  run({}, false, Method::GET, true);
}