// USERVER
#include <userver/tracing/manager.hpp>

// THIS
#include <UServerUtils/Grpc/Http/Server/HttpServerBuilder.hpp>

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
}

void HttpServerBuilder::add_handler(
  HttpHandler* http_handler,
  TaskProcessor& task_processor)
{
  internal::HttpHandlerImpl_var handler(
    new internal::HttpHandlerImpl(
      http_handler,
      storage_mock_->GetSource(),
      userver::tracing::kDefaultTracingManager,
      statistics_storage_));
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