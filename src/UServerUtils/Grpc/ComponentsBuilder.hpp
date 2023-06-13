#ifndef USERVER_GRPC_FACTORY_HPP
#define USERVER_GRPC_FACTORY_HPP

// STD
#include <deque>
#include <memory>
#include <unordered_map>
#include <unordered_set>

// USERVER
#include <engine/task/task_processor.hpp>
#include <userver/ugrpc/client/queue_holder.hpp>
#include <userver/utils/statistics/storage.hpp>

// THIS
#include <eh/Exception.hpp>
#include <Generics/Uncopyable.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <UServerUtils/Grpc/ClientFactory.hpp>
#include <UServerUtils/Grpc/CobrazzClientFactory.hpp>
#include <UServerUtils/Grpc/CobrazzServerBuilder.hpp>
#include <UServerUtils/Grpc/Config.hpp>
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

  struct ComponentsInfo
  {
    ComponentsInfo() = default;
    ~ComponentsInfo() = default;

    ComponentsInfo(const ComponentsInfo&) = delete;
    ComponentsInfo(ComponentsInfo&&) = default;
    ComponentsInfo& operator=(const ComponentsInfo&) = delete;
    ComponentsInfo& operator=(ComponentsInfo&&) = default;

    StatisticsStoragePtr statistics_storage;
    Components components;
    QueueHolders queue_holders;
    NameToUserComponent name_to_user_component;
  };

public:
  explicit ComponentsBuilder();

  ~ComponentsBuilder() = default;

  StatisticsStorage& get_statistics_storage();

  CompletionQueue& add_grpc_server(
    std::unique_ptr<GrpcServerBuilder>&& builder);

  void add_grpc_cobrazz_server(
    std::unique_ptr<GrpcCobrazzServerBuilder>&& builder);

  GrpcClientFactory_var add_grpc_client_factory(
    GrpcClientFactoryConfig&& config,
    TaskProcessor& channel_task_processor,
    grpc::CompletionQueue* queue = nullptr);

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

  QueueHolders queue_holders_;

  GrpcServers grpc_servers_;

  GrpcCobrazzServers grpc_cobrazz_servers_;

  Components components_;

  NameToUserComponent name_to_user_component_;

  CashExistingComponent cash_existing_component_;
};

using ComponentsBuilderPtr = std::unique_ptr<ComponentsBuilder>;

} // namespace UServerUtils::Grpc

#endif //USERVER_GRPC_FACTORY_HPP
