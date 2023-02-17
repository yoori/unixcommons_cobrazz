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

namespace UServerUtils
{
  struct Container
  {
    std::mutex mutex;
    std::map<std::string, unsigned long> vals_ul;
    std::map<std::string, std::string> vals_string;
  };

  static Container container;

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

  /*
  struct StActive
  {
    bool *_b;

    StActive(bool* b): _b(b) {*_b=true;}

    ~StActive() {*_b=false;}
  };
  */

  void copy_json_to_tmp();

  class MetricsHTTPProviderImpl:
    public Generics::SimpleActiveObject,
    public ReferenceCounting::AtomicImpl
  {
  public:
    MetricsHTTPProviderImpl(unsigned int listen_port, std::string uri)
      : listen_port_(listen_port),
        uri_(std::move(uri))
    {}

    ~MetricsHTTPProviderImpl()
    {}

    void
    set_value(std::string_view key, std::string_view value)
    {
      std::lock_guard<std::mutex> g(container.mutex);
      container.vals_string[std::string(key)] = value;
    }

    void
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
    }

    static void* worker(MetricsHTTPProviderImpl* _this)
    {
      const components::ComponentList component_list = components::MinimalServerComponentList()
        .Append<ConfigDistributor>();

      // crypto::impl::Openssl::Init();

      auto conf_replaced = std::regex_replace(
        MetricsHTTPProvider::config, std::regex("~port~"), std::to_string(_this->listen_port_));
      conf_replaced = std::regex_replace(conf_replaced, std::regex("~uri~"), std::string(_this->uri_));
      auto conf_prepared = std::make_unique<components::ManagerConfig>(
        components::ManagerConfig::FromString(conf_replaced, {}, {}));
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

      while(_this->active())
      {
        sleep(1);
      }

      return NULL;
    }

    // override SimpleActiveObject hooks
    void
    activate_object_()
    {
      copy_json_to_tmp();
      thread_ = std::thread(worker, this);
    }

    void
    wait_object_()
    {
      thread_.join();
    }

  private:
    unsigned int listen_port_;
    std::string uri_;
    std::thread thread_;
  };

  MetricsHTTPProvider::MetricsHTTPProvider(unsigned int listen_port, std::string uri)
    : impl_(new MetricsHTTPProviderImpl(listen_port, uri))
  {}

  MetricsHTTPProvider::~MetricsHTTPProvider()
  {}

  void MetricsHTTPProvider::set_value(std::string_view key, std::string_view value)
  {
    impl_->set_value(key, value);
  }

  void MetricsHTTPProvider::add_value(std::string_view key, unsigned long value)
  {
    impl_->add_value(key, value);
  }

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
    return impl_->active();
  }

  ConfigDistributor::ConfigDistributor(
    const components::ComponentConfig& config, const components::ComponentContext& context)
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
      std::lock_guard<std::mutex> lock(container.mutex);
      for(auto&[k,v]: container.vals_string)
      {
        j[std::string(k)] = v;
      }

      for(auto&[k,v]: container.vals_ul)
      {
        j[std::string(k)] = v;
      }
    }

    return j.ExtractValue();
  }
}
