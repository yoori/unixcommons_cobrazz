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

namespace UServerUtils
{
/*  struct Container
  {
    std::mutex mutex;
    std::map<std::string, unsigned long> vals_ul;
    std::map<std::string, std::string> vals_string;
  };*/

  //static Container container;
  static ReferenceCounting::SmartPtr<MetricsProvider> container;
  

  class ConfigDistributor final: public server::handlers::HttpHandlerJsonBase
  {
  public:
    static constexpr std::string_view kName = "handler-config";

    // Component is valid after construction and is able to accept requests
    ConfigDistributor(const components::ComponentConfig& config,
      const components::ComponentContext& context);

    formats::json::Value HandleRequestJsonThrow(
      const server::http::HttpRequest&, const formats::json::Value& json,
      server::request::RequestContext&) const override;
  };

  struct StActive
  {
    bool *_b;

    StActive(bool* b): _b(b) {*_b=true;}

    ~StActive() {*_b=false;}
  };

  void copy_json_to_tmp();

  class MetricsHTTPProviderImpl
  {
  public:
    unsigned int listen_port;
    std::string_view uri;
    std::thread thread_;
    bool stopped_=false;
    bool active_;
    ReferenceCounting::SmartPtr<MetricsProvider> metricsProvider_;


    MetricsHTTPProviderImpl(MetricsProvider *mProv,unsigned int _listen_port, std::string_view _uri)
      : listen_port(_listen_port),uri(_uri), metricsProvider_(mProv)
    {
      printf("KALL %s %d\n",__FILE__,__LINE__);
	container=mProv;
    }

    ~MetricsHTTPProviderImpl()
    {
      printf("KALL %s %d\n",__FILE__,__LINE__);
      if(active_)
      {
        LOG_ERROR() << "Try to destruct active object MetricsHTTPProviderImpl";
      }
    }

/*    void
    set_value(std::string_view key, std::string_view value)
    {
      std::lock_guard<std::mutex> g(container.mutex);
      container.vals_string[std::string(key)] = value;
    }*/

/*    void
    add_value(std::string_view key, unsigned long value)
    {
      std::lock_guard<std::mutex> g(container.mutex);
      auto _key = std::string(key);
      auto it = container.vals_ul.find(_key);
      if(it == container.vals_ul.end())
      {
        container.vals_ul[std::string(key)] = value;
      }
      else
      {
        it->second += value;
      }

      // container.vals_ul[std::string(key)]+=value;
    }*/

    static void* worker(MetricsHTTPProviderImpl* _this)
    {
      printf("KALL %s %d\n",__FILE__,__LINE__);
      const components::ComponentList component_list = components::MinimalServerComponentList()
        .Append<ConfigDistributor>();

      // crypto::impl::Openssl::Init();

      auto conf_replaced = std::regex_replace(config_z,std::regex("~port~"), std::to_string(_this->listen_port));
      conf_replaced = std::regex_replace(conf_replaced,std::regex("~uri~"), std::string(_this->uri));
      auto conf_prepared = std::make_unique<components::ManagerConfig>(components::ManagerConfig::FromString(conf_replaced, {}, {}));
      std::optional<components::Manager> manager;
      printf("KALL %s %d\n",__FILE__,__LINE__);

      try
      {
      printf("KALL %s %d\n",__FILE__,__LINE__);
        manager.emplace(std::move(conf_prepared), component_list);
      printf("KALL %s %d\n",__FILE__,__LINE__);
      }
      catch (const std::exception& ex)
      {
        LOG_ERROR() << "Loading failed: " << ex;
//        throw;
      }
      printf("KALL %s %d\n",__FILE__,__LINE__);

      StActive __act(&_this->active_);

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
    activate_object()
    {
      printf("KALL %s %d\n",__FILE__,__LINE__);
      copy_json_to_tmp();
      thread_ = std::thread(worker,this);
    }

    void
    deactivate_object()
    {
      printf("KALL %s %d\n",__FILE__,__LINE__);
      stopped_=true;
    }

    void
    wait_object()
    {
      printf("KALL %s %d\n",__FILE__,__LINE__);
      thread_.join();
    
    }

    bool
    active()
    {
      return active_;
    }
  };

  MetricsHTTPProvider::MetricsHTTPProvider(MetricsProvider* mProv,unsigned int _listen_port, std::string _uri)
  //: listen_port(_listen_port),uri(_uri)
  {
      printf("KALL %s %d\n",__FILE__,__LINE__);
    impl_.reset(new MetricsHTTPProviderImpl(mProv,_listen_port, _uri));
  }

  MetricsHTTPProvider::~MetricsHTTPProvider()
  {}

//  void MetricsHTTPProvider::set_value(std::string_view key, std::string_view value)
//  {
//    impl_->set_value(key,value);
//  }

//  void MetricsHTTPProvider::add_value(std::string_view key, unsigned long value)
//  {
//    impl_->add_value(key,value);
//  }

  void MetricsHTTPProvider::activate_object()
  {
    impl_->activate_object();
  }

  void MetricsHTTPProvider::deactivate_object()
  {
    impl_->deactivate_object();
  }

  void MetricsHTTPProvider::wait_object()
  {
    impl_->wait_object();
  }

  bool
  MetricsHTTPProvider::active()
  {
    return impl_->active_;
  }

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
      //std::lock_guard<std::mutex> lock(container.mutex);
      auto p=dynamic_cast<CompositeMetricsProvider*>(container.operator->());
      if(!p)
        throw std::runtime_error("invalid cast");
      auto vals=p->getStringValues();//provider
      for(auto&[k,v]: vals)
      {
          j[k]=v;
      }

    }

    return j.ExtractValue();
  }
}
