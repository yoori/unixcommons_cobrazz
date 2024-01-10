// THIS
#include <UServerUtils/Grpc/Http/Server/HttpHandler.hpp>

namespace UServerUtils::Http::Server
{

HttpHandler::HttpHandler(
  const std::string& handler_name,
  const HandlerConfig& handler_config,
  const std::optional<Level> log_level)
  : handler_name_(handler_name),
    handler_config_(handler_config),
    log_level_(log_level)
{
}

const std::string& HttpHandler::handler_name() const noexcept
{
  return handler_name_;
}

const HandlerConfig&
HttpHandler::handler_config() const noexcept
{
  return handler_config_;
}

const std::optional<userver::logging::Level>&
HttpHandler::log_level() const noexcept
{
  return log_level_;
}

namespace internal
{

HttpHandlerImpl::HttpHandlerImpl(
  HttpHandler* http_handler,
  const DynamicConfigSource& dynamic_config_source,
  const TracingManagerBase& tracing_manager,
  StatisticsStorage& statistics_storage)
  : HttpHandlerBase(
      http_handler->handler_name(),
      http_handler->handler_config(),
      dynamic_config_source,
      tracing_manager,
      statistics_storage,
      false,
      http_handler->log_level(),
      false),
    http_handler_(ReferenceCounting::add_ref(http_handler))
{
  add_child_object(http_handler_.in());
}

std::string HttpHandlerImpl::HandleRequestThrow(
  const HttpRequest& request,
  RequestContext& context) const
{
  return http_handler_->handle_request_throw(
    request,
    context);
}

} // namespace internal

} // namespace UServerUtils::Http::Server