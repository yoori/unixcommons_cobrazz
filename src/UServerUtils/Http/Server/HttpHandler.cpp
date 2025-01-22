// THIS
#include <UServerUtils/Http/Server/HttpHandler.hpp>

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

std::string HttpHandler::handle_request_throw(
  const HttpRequest& /*request*/,
  RequestContext& /*context*/) const
{
  return {};
}

void HttpHandler::handle_stream_request(
  const HttpRequest& /*http_request*/,
  RequestContext& /*context*/,
  ResponseBodyStream& /*response_body_stream*/) const
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

namespace details
{

HttpHandlerImpl::HttpHandlerImpl(
  HttpHandler* http_handler,
  const DynamicConfigSource& dynamic_config_source,
  StatisticsStorage& statistics_storage,
  HttpMiddlewares&& http_middlewares,
  const bool is_monitor)
  : HttpHandlerBase(
      http_handler->handler_name(),
      http_handler->handler_config(),
      dynamic_config_source,
      statistics_storage,
      false,
      http_handler->log_level(),
      is_monitor,
      std::move(http_middlewares)),
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

void HttpHandlerImpl::HandleStreamRequest(
  HttpRequest& http_request,
  RequestContext& context,
  ResponseBodyStream& response_body_stream) const
{
  http_handler_->handle_stream_request(
    http_request,
    context,
    response_body_stream);
}

} // namespace internal

} // namespace UServerUtils::Http::Server