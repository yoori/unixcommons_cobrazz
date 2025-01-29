#ifndef USERVER_HTTP_SERVER_HTTPHANDLERBASE_HPP
#define USERVER_HTTP_SERVER_HTTPHANDLERBASE_HPP

// USERVER
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/request/request_base.hpp>
#include <userver/server/request/request_context.hpp>
#include <userver/server/http/http_response_body_stream.hpp>

// THIS
#include <UServerUtils/Http/Server/Config.hpp>
#include <UServerUtils/Component.hpp>

namespace UServerUtils::Http::Server
{

class HttpServerBuilder;

class HttpHandler : public UServerUtils::Component
{
public:
  using Level = userver::logging::Level;
  using HttpRequest = userver::server::http::HttpRequest;
  using RequestContext = userver::server::request::RequestContext;
  using ResponseBodyStream = userver::server::http::ResponseBodyStream;

public:
  virtual std::string handle_request_throw(
    const HttpRequest& request,
    RequestContext& context) const;

  virtual void handle_stream_request(
    const HttpRequest& http_request,
    RequestContext& context,
    ResponseBodyStream& response_body_stream) const;

  const std::string& handler_name() const noexcept;

  const HandlerConfig& handler_config() const noexcept;

  const std::optional<Level>& log_level() const noexcept;

protected:
  HttpHandler(
    const std::string& handler_name,
    const HandlerConfig& handler_config,
    const std::optional<Level> log_level = {});

  ~HttpHandler() override = default;

private:
  const std::string handler_name_;

  const HandlerConfig handler_config_;

  const std::optional<Level> log_level_;
};

using HttpHandler_var = ReferenceCounting::SmartPtr<HttpHandler>;

namespace details
{

class HttpHandlerImpl final :
  public UServerUtils::Component,
  public userver::server::handlers::HttpHandlerBase,
  public ReferenceCounting::AtomicImpl
{
public:
  using HttpHandler_var = ReferenceCounting::SmartPtr<HttpHandler>;
  using DynamicConfigSource = userver::dynamic_config::Source;
  using StatisticsStorage = userver::utils::statistics::Storage;
  using HttpRequest = userver::server::http::HttpRequest;
  using RequestContext = userver::server::request::RequestContext;
  using ResponseBodyStream = userver::server::http::ResponseBodyStream;
  using HttpMiddlewares = userver::server::handlers::HttpHandlerBase::HttpMiddlewares;

public:
  std::string HandleRequestThrow(
    const HttpRequest& request,
    RequestContext& context) const override;

  void HandleStreamRequest(
    HttpRequest& http_request,
    RequestContext& context,
    ResponseBodyStream& response_body_stream) const override;

protected:
  ~HttpHandlerImpl() override = default;

private:
  HttpHandlerImpl(
    HttpHandler* http_handler,
    const DynamicConfigSource& dynamic_config_source,
    StatisticsStorage& statistics_storage,
    HttpMiddlewares&& http_middlewares,
    const bool is_monitor);

private:
  friend class UServerUtils::Http::Server::HttpServerBuilder;

  const HttpHandler_var http_handler_;
};

using HttpHandlerImpl_var = ReferenceCounting::SmartPtr<HttpHandlerImpl>;

} // namespace internal

} // namespace UServerUtils::Http

#endif //USERVER_HTTP_SERVER_HTTPHANDLERBASE_HPP