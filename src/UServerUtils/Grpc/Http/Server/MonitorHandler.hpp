#ifndef USERVER_HTTP_SERVER_MONITORHANDLER_HPP
#define USERVER_HTTP_SERVER_MONITORHANDLER_HPP

// USERVER
#include <userver/utils/statistics/storage.hpp>

// THIS
#include <UServerUtils/Grpc/Http/Server/Config.hpp>
#include <UServerUtils/Grpc/Http/Server/HttpHandler.hpp>

namespace UServerUtils::Http::Server
{

class MonitorHandler final  :
  public HttpHandler,
  public ReferenceCounting::AtomicImpl
{
public:
  using StatisticsStorage = userver::utils::statistics::Storage;

public:
  MonitorHandler(
    StatisticsStorage& statistics_storage,
    const std::string& handler_name,
    const HandlerConfig& handler_config,
    const std::optional<Level> log_level = {});

  std::string handle_request_throw(
    const HttpRequest& request,
    RequestContext& context) const override;

private:
  ~MonitorHandler() override = default;

private:
  StatisticsStorage& statistics_storage_;
};

} // namespace UServerUtils::Http::Server

#endif // USERVER_HTTP_SERVER_MONITORHANDLER_HPP