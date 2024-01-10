#ifndef USERVER_HTTP_SERVER_CONFIG_HPP
#define USERVER_HTTP_SERVER_CONFIG_HPP

// USERVER
#include <userver/server/request/request_config.hpp>
#include <userver/server/request/request_config.hpp>
#include <userver/server/handlers/handler_config.hpp>
#include <server/server_config.hpp>

namespace UServerUtils::Http::Server
{

using HandlerConfig = userver::server::handlers::HandlerConfig;
using HttpRequestConfig = userver::server::request::HttpRequestConfig;
using ListenerConfig = userver::server::net::ListenerConfig;

struct ServerConfig final
{
  ServerConfig() = default;
  ~ServerConfig() = default;

  std::string server_name = "HttpServer";
  ListenerConfig listener_config;
};

} // namespace UServerUtils::Http

#endif //USERVER_HTTP_SERVER_CONFIG_HPP