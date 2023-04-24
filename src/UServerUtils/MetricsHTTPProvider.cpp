#include <string>
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
#include <regex>

#include "MetricsHTTPProvider.hpp"
#include "Generics/CompositeMetricsProvider.hpp"
#include "ConfigDistributor.hpp"

namespace UServerUtils
{
void copy_json_to_tmp();
ReferenceCounting::SmartPtr<MetricsProvider> MetricsHTTPProvider::container;

void* MetricsHTTPProvider::worker(MetricsHTTPProvider* _this)
{





    const components::ComponentList component_list = components::MinimalServerComponentList()
            .Append<ConfigDistributor>();

    auto conf_replaced = std::regex_replace(config_z_yaml,std::regex("~port~"), std::to_string(_this->listen_port_));
    conf_replaced = std::regex_replace(conf_replaced,std::regex("~uri~"), std::string(_this->uri_));
    auto conf_prepared = std::make_unique<components::ManagerConfig>(components::ManagerConfig::FromString(conf_replaced, {}, {}));
    std::optional<components::Manager> manager;

    try
    {
        manager.emplace(std::move(conf_prepared), component_list);
        _this->state_=AS_ACTIVE;
    }
    catch (const std::exception& ex)
    {
        LOG_ERROR() << "Loading failed: " << ex;
    }




    while(true)
    {
        if(_this->stopped_)
        {
            return NULL;
        }

        sleep(1);
    }
    return NULL;
}

void
MetricsHTTPProvider::activate_object()
{
    copy_json_to_tmp();
    thread_ = std::thread(worker,this);
}

void
MetricsHTTPProvider::deactivate_object()
{
    state_=AS_DEACTIVATING;
    stopped_=true;
}

void
MetricsHTTPProvider::wait_object()
{
    thread_.join();
    state_=AS_NOT_ACTIVE;
}

bool
MetricsHTTPProvider::active()
{
    return state_==AS_ACTIVE;
}



MetricsHTTPProvider::MetricsHTTPProvider(MetricsProvider *mProv,unsigned int _listen_port, std::string_view _uri)
    : listen_port_(_listen_port),uri_(_uri)
{


    container=mProv;
    state_=AS_NOT_ACTIVE;
}

MetricsHTTPProvider::~MetricsHTTPProvider()
{

    if(state_!=AS_NOT_ACTIVE)
    {
        LOG_ERROR() << "Try to destruct active object MetricsHTTPProviderImpl";
    }
}


}
