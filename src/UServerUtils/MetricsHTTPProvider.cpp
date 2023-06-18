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

  ReferenceCounting::SmartPtr<Generics::MetricsProvider> MetricsHTTPProvider::container;

  void* MetricsHTTPProvider::worker(MetricsHTTPProvider* _this)
  {
    // TO FIX: catch eh::exception globally for exclude crashing by unexpected exception if it will be raised by MinimalServerComponentList, ManagerConfig, ...

    const components::ComponentList component_list = components::MinimalServerComponentList()
      .Append<ConfigDistributor>();

    auto conf_replaced = std::regex_replace(config_z_yaml,std::regex("~port~"), std::to_string(_this->listen_port_));
    conf_replaced = std::regex_replace(conf_replaced,std::regex("~uri~"), std::string(_this->uri_));
    auto conf_prepared = std::make_unique<components::ManagerConfig>(components::ManagerConfig::FromString(conf_replaced, {}, {}));
    std::optional<components::Manager> manager;

    try
    {
      manager.emplace(std::move(conf_prepared), component_list);
//      _this->activate_object();
    }
    catch (const eh::Exception& ex)
    {
      LOG_ERROR() << "Loading failed: " << ex;
    }
    catch(...)
    {
      LOG_ERROR() << "Catchet unknown exception";
    }

    // TO FIX : make it normally - conditional variable with waiting
      Sync::ConditionalGuard guard(_this->cond_);
      while (_this->active())
      {
        guard.wait();
      }


    return NULL;
  }

  void
  MetricsHTTPProvider::activate_object()
  {
    SimpleActiveObject::activate_object();
    copy_json_to_tmp();
    thread_ = std::thread(worker,this);
  }

  void
  MetricsHTTPProvider::deactivate_object()
  {
    SimpleActiveObject::deactivate_object();
  }

  void
  MetricsHTTPProvider::wait_object()
  {
//    Sync::PosixGuard guard(cond_);
/*    if (state_ != AS_ACTIVE)
    {
      return;
    }
    state_ = AS_DEACTIVATING;*/
    cond_.broadcast();

    thread_.join();
    SimpleActiveObject::wait_object();
  }


  MetricsHTTPProvider::MetricsHTTPProvider(Generics::MetricsProvider *mProv,unsigned int _listen_port, std::string _uri)
    : listen_port_(_listen_port),
      uri_(std::move(_uri))
  {
    container = ReferenceCounting::add_ref(mProv);
//    state_ = AS_NOT_ACTIVE;
  }

  MetricsHTTPProvider::~MetricsHTTPProvider()
  {
//    if(state_ != AS_NOT_ACTIVE)
//    {
//      LOG_ERROR() << "Try to destruct active object MetricsHTTPProviderImpl";
//    }
  }
}
