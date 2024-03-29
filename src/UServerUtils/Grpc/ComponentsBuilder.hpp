#ifndef USERVER_GRPC_FACTORY_HPP
#define USERVER_GRPC_FACTORY_HPP

// STD
#include <deque>
#include <memory>
#include <unordered_map>
#include <unordered_set>

// USERVER
#include <userver/engine/task/task_processor.hpp>
#include <userver/ugrpc/client/queue_holder.hpp>
#include <userver/utils/statistics/storage.hpp>

// THIS
#include <eh/Exception.hpp>
#include <Generics/Uncopyable.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <UServerUtils/Grpc/Http/Server/HttpServerBuilder.hpp>
#include <UServerUtils/Grpc/Statistics/StatisticsProvider.hpp>
#include <UServerUtils/Grpc/ClientFactory.hpp>
#include <UServerUtils/Grpc/CobrazzClientFactory.hpp>
#include <UServerUtils/Grpc/CobrazzServerBuilder.hpp>
#include <UServerUtils/Grpc/Config.hpp>
#include <UServerUtils/Grpc/RegistratorDynamicSettings.hpp>
#include <UServerUtils/Grpc/Server.hpp>
#include <UServerUtils/Grpc/ServerBuilder.hpp>
#include <UServerUtils/Grpc/ServiceBase.hpp>

namespace UServerUtils::Grpc
{

class ComponentsBuilder final
  : private Generics::Uncopyable
{
public:
  using TaskProcessor = userver::engine::TaskProcessor;
  using StatisticsStorage = userver::utils::statistics::Storage;
  using StatisticsStoragePtr = std::unique_ptr<StatisticsStorage>;
  using CompletionQueue = grpc::CompletionQueue;
  using Components = std::deque<Component_var>;
  using MiddlewareFactories = userver::ugrpc::client::MiddlewareFactories;
  using StatisticsProvider = UServerUtils::Statistics::StatisticsProvider;
  using StatisticsProviderPtr = UServerUtils::Statistics::StatisticsProviderPtr;

  struct StatisticsProviderInfo
  {
    std::string statistics_prefix;
    StatisticsProviderPtr statistics_provider;
  };

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

private:
  using GrpcCobrazzServer_var = Core::Server::ServerCoro_var;
  using GrpcCobrazzServers = std::deque<GrpcCobrazzServer_var>;
  using GrpcServers = std::deque<GrpcServer_var>;
  using QueueHolder = userver::ugrpc::client::QueueHolder;
  using QueueHolderPtr = std::unique_ptr<QueueHolder>;
  using QueueHolders = std::deque<QueueHolderPtr>;
  using CashExistingComponent = std::unordered_set<std::uintptr_t>;
  using NameToUserComponent = std::unordered_map<std::string, Component_var>;
  using Middlewares = userver::ugrpc::server::Middlewares;
  using MiddlewaresPtr = std::unique_ptr<Middlewares>;
  using MiddlewaresList = std::list<MiddlewaresPtr>;
  using HttpServer = UServerUtils::Http::Server::HttpServer;
  using HttpServer_var = UServerUtils::Http::Server::HttpServer_var;
  using HttpServers = std::deque<HttpServer_var>;
  using HttpServerBuilder = Http::Server::HttpServerBuilder;
  using StatisticsHolder = userver::utils::statistics::Entry;
  using StatisticsHolderPtr = std::unique_ptr<StatisticsHolder>;

  struct ComponentsInfo
  {
    ComponentsInfo() = default;
    ~ComponentsInfo() = default;

    ComponentsInfo(const ComponentsInfo&) = delete;
    ComponentsInfo(ComponentsInfo&&) = default;
    ComponentsInfo& operator=(const ComponentsInfo&) = delete;
    ComponentsInfo& operator=(ComponentsInfo&&) = default;

    MiddlewaresList middlewares_list;
    QueueHolders queue_holders;
    StatisticsStoragePtr statistics_storage;
    Components components;
    NameToUserComponent name_to_user_component;
    StatisticsHolderPtr statistics_holder;
  };

public:
  explicit ComponentsBuilder(
    std::optional<StatisticsProviderInfo> statistics_provider_info = {});

  ~ComponentsBuilder() = default;

  StatisticsStorage& get_statistics_storage();

  CompletionQueue& add_grpc_server(
    std::unique_ptr<GrpcServerBuilder>&& builder);

  GrpcClientFactory_var add_grpc_client_factory(
    GrpcClientFactoryConfig&& config,
    TaskProcessor& channel_task_processor,
    grpc::CompletionQueue* queue = nullptr,
    const MiddlewareFactories& middleware_factories = {});

  void add_grpc_cobrazz_server(
    std::unique_ptr<GrpcCobrazzServerBuilder>&& builder);

  void add_http_server(
    std::unique_ptr<HttpServerBuilder>&& builder);

  void add_user_component(
    const std::string& name,
    Component* component);

private:
  ComponentsInfo build();

  void add_component(Component* component);

  bool check_component_cash(Component* component);

  void add_component_cash(Component* component);

private:
  friend class Manager;

  StatisticsStoragePtr statistics_storage_;

  StatisticsProviderPtr statistics_provider_;

  std::string statistics_prefix_;

  QueueHolders queue_holders_;

  GrpcServers grpc_servers_;

  MiddlewaresList middlewares_list_;

  GrpcCobrazzServers grpc_cobrazz_servers_;

  HttpServers http_servers_;

  Components components_;

  NameToUserComponent name_to_user_component_;

  CashExistingComponent cash_existing_component_;
};

using ComponentsBuilderPtr = std::unique_ptr<ComponentsBuilder>;

} // namespace UServerUtils::Grpc

#endif //USERVER_GRPC_FACTORY_HPP