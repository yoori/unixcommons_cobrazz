// THIS
#include <UServerUtils/Grpc/Http/HttpHandler.hpp>

namespace UServerUtils::Http
{

HttpHandler::HttpHandler(
  const std::string& handler_name,
  const HandlerConfig& handler_config,
  const bool is_body_streamed,
  const std::optional<Level> log_level)
  : handler_name_(handler_name),
    handler_config_(handler_config),
    is_body_streamed_(is_body_streamed),
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

bool HttpHandler::is_body_streamed() const noexcept
{
  return is_body_streamed_;
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
      http_handler->is_body_streamed(),
      http_handler->log_level(),
      false),
    http_handler_(ReferenceCounting::add_ref(http_handler))
{
  add_child_object(http_handler_.in());
}

void HttpHandlerImpl::HandleRequest(
  userver::server::request::RequestBase& request,
  userver::server::request::RequestContext& context) const
{
  http_handler_->handle_request(request, context);
}

} // namespace internal

} // namespace UServerUtils::Http