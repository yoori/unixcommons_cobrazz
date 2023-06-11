#pragma once

#include <string>
#include <regex>

#include <userver/components/minimal_server_component_list.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/utils/daemon_run.hpp>
#include <userver/utils/datetime.hpp>
#include <userver/components/run.hpp>
#include <userver/formats/json.hpp>
#include <userver/components/manager.hpp>
#include <userver/components/manager_config.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/logging/log.hpp>

#include "MetricsHTTPProvider.hpp"
#include "Generics/CompositeMetricsProvider.hpp"
#include "ConfigDistributor.hpp"

namespace UServerUtils
{
  class ConfigDistributor final: public server::handlers::HttpHandlerBase
  {
  public:
    static constexpr std::string_view kName = "handler-config";

    // Component is valid after construction and is able to accept requests
    ConfigDistributor(const components::ComponentConfig& config,
      const components::ComponentContext& context);

    std::string HandleRequestThrow(
      const server::http::HttpRequest&,
      server::request::RequestContext&) const override;
  };
}
