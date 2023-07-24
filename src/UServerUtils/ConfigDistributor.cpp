#include <string>
#include <regex>

#include <userver/components/minimal_server_component_list.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/utils/daemon_run.hpp>
#include <userver/utils/datetime.hpp>
#include <userver/components/run.hpp>
#include <userver/formats/json.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/logging/log.hpp>

#include "MetricsHTTPProvider.hpp"
#include "Generics/CompositeMetricsProvider.hpp"
#include "ConfigDistributor.hpp"

namespace UServerUtils
{
    ConfigDistributor::ConfigDistributor(
        const components::ComponentConfig& config,
        const components::ComponentContext& context) :
        HttpHandlerBase(config, context)
    {}

    std::string
    ConfigDistributor::HandleRequestThrow(
        const server::http::HttpRequest& r,
        server::request::RequestContext&) const
    {
        auto p =  MetricsHTTPProvider::container.operator->();


        bool isJson = r.HasArg("json");

        if(isJson)
        {
            auto vals = p->getStringValues(); //provider
            formats::json::ValueBuilder j;
            for(auto&[k,v]: vals)
            {
                j[k] = v;
            }

            return ToString(j.ExtractValue());
        }
        else
        {
            auto s = p->get_prometheus_formatted();
            return s;
        }
    }
}
