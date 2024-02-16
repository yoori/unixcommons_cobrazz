// USERVER
#include <userver/tracing/manager.hpp>

// THIS
#include <UServerUtils/Grpc/Http/Server/HttpServerBuilder.hpp>
#include <UServerUtils/Grpc/Http/Server/MonitorHandler.hpp>

namespace UServerUtils::Http::Server
{

HttpServerBuilder::HttpServerBuilder(
  Logger* logger,
  const ServerConfig& config,
  TaskProcessor& listener_task_processor,
  StatisticsStorage& statistics_storage)
  : statistics_storage_(statistics_storage)
{
  const auto& docs_map = registrator_dynamic_settings_.docs_map();
  storage_mock_ = StorageMockPtr(new StorageMock(
    docs_map,
    {}));

  http_server_ = HttpServer_var(
    new HttpServer(
      logger,
      config,
      listener_task_processor,
      statistics_storage,
      storage_mock_));

  if (config.monitor_listener_config.has_value())
  {
    HandlerConfig handler_config;
    handler_config.method = "GET";
    handler_config.path = "/metrics";
    handler_config.response_body_stream = false;
    ReferenceCounting::SmartPtr<MonitorHandler> monitor_handler(
      new MonitorHandler(
        statistics_storage,
        "MonitorHandler",
        handler_config));

    add_handler(monitor_handler.in(), listener_task_processor, true);
  }
}

void HttpServerBuilder::add_handler(
  HttpHandler* http_handler,
  TaskProcessor& task_processor,
  const bool is_monitor)
{
  internal::HttpHandlerImpl_var handler(
    new internal::HttpHandlerImpl(
      http_handler,
      storage_mock_->GetSource(),
      userver::tracing::kDefaultTracingManager,
      statistics_storage_,
      is_monitor));
  http_server_->add_handler(handler.in(), task_processor);
  http_handlers_.emplace_back(
    Component_var(
      ReferenceCounting::add_ref(handler.in())));
}

HttpServerBuilder::ServerInfo HttpServerBuilder::build()
{
  ServerInfo server_info;
  server_info.http_server = std::move(http_server_);
  server_info.http_handlers = std::move(http_handlers_);
  return server_info;
}

} // namespace UServerUtils::Http::Server