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
  void copy_json_to_tmp();
    ReferenceCounting::SmartPtr<MetricsProvider> MetricsHTTPProvider::container;

    void* MetricsHTTPProvider::worker(MetricsHTTPProvider* _this)
    {
    
    

printf("KALL %s %d\n",__FILE__,__LINE__);
      
      const components::ComponentList component_list = components::MinimalServerComponentList()
        .Append<ConfigDistributor>();

      // crypto::impl::Openssl::Init();

      auto conf_replaced = std::regex_replace(config_z_yaml,std::regex("~port~"), std::to_string(_this->listen_port_));
      conf_replaced = std::regex_replace(conf_replaced,std::regex("~uri~"), std::string(_this->uri_));
      auto conf_prepared = std::make_unique<components::ManagerConfig>(components::ManagerConfig::FromString(conf_replaced, {}, {}));
      std::optional<components::Manager> manager;
printf("KALL %s %d\n",__FILE__,__LINE__);
      

      try
      {
      
        manager.emplace(std::move(conf_prepared), component_list);
        _this->state_=AS_ACTIVE;
      
      }
      catch (const std::exception& ex)
      {
        LOG_ERROR() << "Loading failed: " << ex;
      }
      
printf("KALL %s %d\n",__FILE__,__LINE__);


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
      
printf("KALL %s %d\n",__FILE__,__LINE__);
      copy_json_to_tmp();
      thread_ = std::thread(worker,this);
    }

    void
    MetricsHTTPProvider::deactivate_object()
    {
printf("KALL %s %d\n",__FILE__,__LINE__);
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
//     AS_ACTIVE,
//      AS_DEACTIVATING,
//      AS_NOT_ACTIVE



    MetricsHTTPProvider::MetricsHTTPProvider(MetricsProvider *mProv,unsigned int _listen_port, std::string_view _uri)
      : listen_port_(_listen_port),uri_(_uri)//, metricsProvider_(mProv)
    {
printf("KALL %s %d\n",__FILE__,__LINE__);
      
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
