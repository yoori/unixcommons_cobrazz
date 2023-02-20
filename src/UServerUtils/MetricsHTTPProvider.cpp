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
/*  struct Container
  {
    std::mutex mutex;
    std::map<std::string, unsigned long> vals_ul;
    std::map<std::string, std::string> vals_string;
  };*/

  //static Container container;
  ReferenceCounting::SmartPtr<MetricsProvider> container;
  

/*
  struct StActive
  {
    bool *_b;

    StActive(bool* b): _b(b) {*_b=true;}

    ~StActive() {*_b=false;}
  };*/

  void copy_json_to_tmp();

    void* MetricsHTTPProvider::worker(MetricsHTTPProvider* _this)
    {
    
    
      
      const components::ComponentList component_list = components::MinimalServerComponentList()
        .Append<ConfigDistributor>();

      // crypto::impl::Openssl::Init();

      auto conf_replaced = std::regex_replace(config_z,std::regex("~port~"), std::to_string(_this->listen_port));
      conf_replaced = std::regex_replace(conf_replaced,std::regex("~uri~"), std::string(_this->uri));
      auto conf_prepared = std::make_unique<components::ManagerConfig>(components::ManagerConfig::FromString(conf_replaced, {}, {}));
      std::optional<components::Manager> manager;
      

      try
      {
      
        manager.emplace(std::move(conf_prepared), component_list);
      
      }
      catch (const std::exception& ex)
      {
        LOG_ERROR() << "Loading failed: " << ex;
//        throw;
      }
      

//      StActive __act(&_this->active_);

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
      
      stopped_=true;
    }
    void
    MetricsHTTPProvider::wait_object()
    {
      
      thread_.join();
    
    }

    bool
    MetricsHTTPProvider::active()
    {
      return active_;
    }
//     AS_ACTIVE,
//      AS_DEACTIVATING,
//      AS_NOT_ACTIVE


  class MetricsHTTPProviderImpl
  {
  public:
    unsigned int listen_port;
    std::string_view uri;
    std::thread thread_;
    bool stopped_=false;
    bool active_;
    ReferenceCounting::SmartPtr<MetricsProvider> metricsProvider_;





  };

    MetricsHTTPProvider::MetricsHTTPProvider(MetricsProvider *mProv,unsigned int _listen_port, std::string_view _uri)
      : listen_port(_listen_port),uri(_uri), metricsProvider_(mProv)
    {
      
	container=mProv;
	state_=AS_NOT_ACTIVE;
    }

    MetricsHTTPProvider::~MetricsHTTPProvider()
    {
      
      if(active_)
      {
        LOG_ERROR() << "Try to destruct active object MetricsHTTPProviderImpl";
      }
    }


/*  MetricsHTTPProvider::MetricsHTTPProvider(MetricsProvider* mProv,unsigned int _listen_port, std::string _uri)
  : impl_(new MetricsHTTPProviderImpl(mProv,_listen_port, _uri))
  {
      
//    impl_.reset(new MetricsHTTPProviderImpl(mProv,_listen_port, _uri));
  }

  MetricsHTTPProvider::~MetricsHTTPProvider()
  {
    delete impl_;
  }

*/
//  void MetricsHTTPProvider::activate_object()
//  {
//    impl_->activate_object();
//  }

//  void MetricsHTTPProvider::deactivate_object()
//  {
//    impl_->deactivate_object();
//  }

/*  void MetricsHTTPProvider::wait_object()
  {
    impl_->wait_object();
  }

  bool
  MetricsHTTPProvider::active()
  {
    return impl_->active_;
  }
*/
}
