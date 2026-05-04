#include "MetricsHTTPProvider.hpp"

#include <string>

#ifdef USERVER_UTILS_NO_HTTP_PROVIDER

namespace UServerUtils
{
  ReferenceCounting::SmartPtr<Generics::MetricsProvider> MetricsHTTPProvider::container;

  void*
  MetricsHTTPProvider::worker(MetricsHTTPProvider*)
  {
    return nullptr;
  }

  void
  MetricsHTTPProvider::activate_object_()
  {}

  void
  MetricsHTTPProvider::wait_object_()
  {}

  MetricsHTTPProvider::MetricsHTTPProvider(
    Generics::MetricsProvider* metrics_provider,
    unsigned int listen_port,
    std::string_view uri)
    : listen_port_(listen_port),
      uri_(uri)
  {
    container = ReferenceCounting::add_ref(metrics_provider);
  }

  MetricsHTTPProvider::~MetricsHTTPProvider()
  {}
}

#else

#include <components/manager.hpp>
#include <components/manager_config.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/utils/daemon_run.hpp>
#include <userver/utils/datetime.hpp>
#include <userver/components/run.hpp>
#include <userver/formats/json.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/logging/log.hpp>
#include <regex>

#include "Generics/CompositeMetricsProvider.hpp"
#include "ConfigDistributor.hpp"

namespace UServerUtils
{
  void copy_json_to_tmp();

  ReferenceCounting::SmartPtr<Generics::MetricsProvider> MetricsHTTPProvider::container;

  void* MetricsHTTPProvider::worker(MetricsHTTPProvider* _this)
  {
    const components::ComponentList component_list = components::MinimalServerComponentList()
      .Append<ConfigDistributor>();

    auto conf_replaced = std::regex_replace(
      config_z_yaml,std::regex("~port~"), std::to_string(_this->listen_port_));
    conf_replaced = std::regex_replace(
      conf_replaced,std::regex("~uri~"), std::string(_this->uri_));
    auto conf_prepared = std::make_unique<components::ManagerConfig>(
      components::ManagerConfig::FromString(conf_replaced, {}, {}));
    std::optional<components::Manager> manager;

    try
    {
      manager.emplace(std::move(conf_prepared), component_list);
      _this->state_ = AS_ACTIVE;
    }
    catch (const std::exception& ex)
    {
      LOG_ERROR() << "Loading failed: " << ex;
    }

    while(_this->active())
    {
      sleep(1);
    }

    return nullptr;
  }

  void
  MetricsHTTPProvider::activate_object_()
  {
    copy_json_to_tmp();
    thread_ = std::thread(worker, this);
  }

  void
  MetricsHTTPProvider::wait_object_()
  {
    thread_.join();
  }

  MetricsHTTPProvider::MetricsHTTPProvider(
    Generics::MetricsProvider* metrics_provider,
    unsigned int listen_port,
    std::string_view uri)
    : listen_port_(listen_port),
      uri_(uri)
  {
    container = ReferenceCounting::add_ref(metrics_provider);
  }

  MetricsHTTPProvider::~MetricsHTTPProvider()
  {
  }
}

#endif
