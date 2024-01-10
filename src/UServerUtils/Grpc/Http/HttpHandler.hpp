#ifndef USERVER_HTTP_HTTPHANDLERBASE_HPP
#define USERVER_HTTP_HTTPHANDLERBASE_HPP

// USERVER
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/request/request_base.hpp>
#include <userver/server/request/request_context.hpp>

// THIS
#include <UServerUtils/Grpc/Http/Config.hpp>
#include <UServerUtils/Grpc/Component.hpp>

namespace UServerUtils::Http
{

class HttpServerBuilder;

class HttpHandler : public UServerUtils::Grpc::Component
{
public:
  using Level = userver::logging::Level;

public:
  virtual void handle_request(
    userver::server::request::RequestBase& request,
    userver::server::request::RequestContext& context) const = 0;

  const std::string& handler_name() const noexcept;

  const HandlerConfig& handler_config() const noexcept;

  bool is_body_streamed() const noexcept;

  const std::optional<Level>& log_level() const noexcept;

protected:
  HttpHandler(
    const std::string& handler_name,
    const HandlerConfig& handler_config,
    const bool is_body_streamed = false,
    const std::optional<Level> log_level = {});

  ~HttpHandler() override = default;

private:
  const std::string handler_name_;

  const HandlerConfig handler_config_;

  const bool is_body_streamed_;

  const std::optional<Level> log_level_;
};

using HttpHandler_var = ReferenceCounting::SmartPtr<HttpHandler>;

namespace internal
{

class HttpHandlerImpl final :
  public UServerUtils::Grpc::Component,
  public userver::server::handlers::HttpHandlerBase,
  public ReferenceCounting::AtomicImpl
{
public:
  using HttpHandler_var = ReferenceCounting::SmartPtr<HttpHandler>;
  using DynamicConfigSource = userver::dynamic_config::Source;
  using TracingManagerBase = userver::tracing::TracingManagerBase;
  using StatisticsStorage = userver::utils::statistics::Storage;

  void HandleRequest(
    userver::server::request::RequestBase& request,
    userver::server::request::RequestContext& context) const override;

protected:
  ~HttpHandlerImpl() override = default;

private:
  HttpHandlerImpl(
    HttpHandler* http_handler,
    const DynamicConfigSource& dynamic_config_source,
    const TracingManagerBase& tracing_manager,
    StatisticsStorage& statistics_storage);

private:
  friend class UServerUtils::Http::HttpServerBuilder;

  const HttpHandler_var http_handler_;
};

using HttpHandlerImpl_var = ReferenceCounting::SmartPtr<HttpHandlerImpl>;

} // namespace internal

} // namespace UServerUtils::Http

#endif //USERVER_HTTP_HTTPHANDLERBASE_HPP