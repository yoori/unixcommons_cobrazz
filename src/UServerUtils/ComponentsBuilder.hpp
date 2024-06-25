#ifndef USERVER_FACTORY_HPP
#define USERVER_FACTORY_HPP

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
#include <UServerUtils/Grpc/Server/ServerBuilder.hpp>
#include <UServerUtils/Grpc/Server/ServiceBase.hpp>
#include <UServerUtils/Http/Server/HttpServerBuilder.hpp>
#include <UServerUtils/Statistics/StatisticsProvider.hpp>
#include <UServerUtils/UServerGrpc/ClientFactory.hpp>
#include <UServerUtils/UServerGrpc/Server.hpp>
#include <UServerUtils/UServerGrpc/ServerBuilder.hpp>
#include <UServerUtils/Config.hpp>
#include <UServerUtils/RegistratorDynamicSettings.hpp>

namespace UServerUtils
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
  using GrpcCobrazzServer_var = Grpc::Server::ServerCoro_var;
  using GrpcCobrazzServers = std::deque<GrpcCobrazzServer_var>;
  using GrpcCobrazzServerBuilder = Grpc::Server::ServerBuilder;
  using GrpcUserverServer = UServerUtils::UServerGrpc::GrpcServer;
  using GrpcUserverServer_var = UServerUtils::UServerGrpc::GrpcServer_var;
  using GrpcUserverServers = std::deque<GrpcUserverServer_var>;
  using GrpcUserverClientFactoryConfig = UServerGrpc::ClientFactoryConfig;
  using GrpcUserverServerBuilder = UServerGrpc::GrpcServerBuilder;
  using GrpcUserverClientFactory = UServerGrpc::GrpcClientFactory;
  using GrpcUserverClientFactory_var = UServerGrpc::GrpcClientFactory_var;
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
  using StatisticsHolders = std::list<StatisticsHolderPtr>;

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
    StatisticsHolders statistics_holders;
  };

public:
  explicit ComponentsBuilder(
    std::optional<StatisticsProviderInfo> statistics_provider_info = {});

  ~ComponentsBuilder() = default;

  StatisticsStorage& get_statistics_storage();

  CompletionQueue& add_grpc_userver_server(
    std::unique_ptr<GrpcUserverServerBuilder>&& builder);

  GrpcUserverClientFactory_var add_grpc_userver_client_factory(
    GrpcUserverClientFactoryConfig&& config,
    TaskProcessor& channel_task_processor,
    grpc::CompletionQueue* queue = nullptr,
    const MiddlewareFactories& middleware_factories = {});

  void add_grpc_cobrazz_server(std::unique_ptr<GrpcCobrazzServerBuilder>&& builder);

  void add_http_server(std::unique_ptr<HttpServerBuilder>&& builder);

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

  GrpcUserverServers grpc_userver_servers_;

  MiddlewaresList middlewares_list_;

  GrpcCobrazzServers grpc_cobrazz_servers_;

  HttpServers http_servers_;

  Components components_;

  NameToUserComponent name_to_user_component_;

  CashExistingComponent cash_existing_component_;
};

using ComponentsBuilderPtr = std::unique_ptr<ComponentsBuilder>;

} // namespace UServerUtils

#endif //USERVER_FACTORY_HPP