#include "ConfigDistributor.hpp"
#include <string>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/utils/daemon_run.hpp>
#include <userver/utils/datetime.hpp>
//#include <crypto/openssl.hpp>
#include <userver/components/run.hpp>
//#include <utils/jemalloc.hpp>
#include <userver/formats/json.hpp>
#include <userver/components/manager.hpp>
#include <userver/components/manager_config.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/logging/log.hpp>
#include <regex>

#include "MetricsHTTPProvider.hpp"
#include "CompositeMetricsProvider.hpp"
#include "ConfigDistributor.hpp"

namespace UServerUtils
{
//    extern ReferenceCounting::SmartPtr<MetricsProvider> container;

  ConfigDistributor::ConfigDistributor(const components::ComponentConfig& config, const components::ComponentContext& context)
    : HttpHandlerJsonBase(config, context)
  {}

  formats::json::Value
  ConfigDistributor::HandleRequestJsonThrow(
    const server::http::HttpRequest&,
    const formats::json::Value& json,
    server::request::RequestContext&) const
  {
    formats::json::ValueBuilder j;

    {
printf("KALL %s %d\n",__FILE__,__LINE__);
      auto p=dynamic_cast<CompositeMetricsProvider*>(MetricsHTTPProvider::container.operator->());
      if(!p)
        throw std::runtime_error("invalid cast");
printf("KALL %s %d\n",__FILE__,__LINE__);
      auto vals=p->getStringValues();//provider
printf("KALL %s %d\n",__FILE__,__LINE__);
      for(auto&[k,v]: vals)
      {
          j[k]=v;
      }

    }

    return j.ExtractValue();
  }


}