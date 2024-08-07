// STD
#include <iostream>

// USERVER
#include <userver/utils/statistics/pretty_format.hpp>
#include <userver/utils/statistics/prometheus.hpp>
#include <userver/utils/statistics/json.hpp>

// THIS
#include <UServerUtils/Http/Server/MonitorHandler.hpp>

namespace UServerUtils::Http::Server
{

MonitorHandler::MonitorHandler(
  StatisticsStorage& statistics_storage,
  const std::string& handler_name,
  const HandlerConfig& handler_config,
  const std::optional<Level> log_level)
  : HttpHandler(handler_name, handler_config, log_level),
    statistics_storage_(statistics_storage)
{
}

std::string MonitorHandler::handle_request_throw(
  const HttpRequest& request,
  RequestContext& context) const
{
  auto& response = request.GetHttpResponse();
  response.SetStatus(userver::server::http::HttpStatus::kOk);

  const auto& format = request.GetArg("format");
  if (format == "json")
  {
    return userver::utils::statistics::ToJsonFormat(
      statistics_storage_,
      {});
  }
  else if (format == "human")
  {
    return userver::utils::statistics::ToPrettyFormat(
      statistics_storage_,
      {});
  }
  else if (format == "prometheus")
  {
    return userver::utils::statistics::ToPrometheusFormatUntyped(
      statistics_storage_,
      {});
  }
  else
  {
    return userver::utils::statistics::ToPrometheusFormatUntyped(
      statistics_storage_,
      {});
  }
}

} // namespace UServerUtils::Http::Server