#ifndef USERVER_USERVERGRPC_CLIENTFACTORY_HPP
#define USERVER_USERVERGRPC_CLIENTFACTORY_HPP

// STD
#include <memory>

// USERVER
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/dynamic_config/storage_mock.hpp>
#include <userver/ugrpc/client/client_factory.hpp>
#include <userver/ugrpc/impl/statistics_storage.hpp>

// GRPCCPP
#include <grpcpp/completion_queue.h>

// THIS
#include <ReferenceCounting/AtomicImpl.hpp>
#include <UServerUtils/UServerGrpc/Config.hpp>
#include <UServerUtils/Config.hpp>
#include <UServerUtils/Component.hpp>
#include <UServerUtils/RegistratorDynamicSettings.hpp>


namespace UServerUtils
{

class ComponentsBuilder;

} // namespace UServerUtils

namespace UServerUtils::UServerGrpc
{

class GrpcClientFactory final :
  public Component,
  public ReferenceCounting::AtomicImpl
{
private:
  using ClientFactory = userver::ugrpc::client::ClientFactory;
  using ClientFactoryPtr = std::unique_ptr<ClientFactory>;
  using TaskProcessor = userver::engine::TaskProcessor;
  using StatisticsStorage = userver::utils::statistics::Storage;
  using GrpcStatisticsStorage = userver::ugrpc::impl::StatisticsStorage;
  using GrpcControl = userver::testsuite::GrpcControl;
  using StorageMock = userver::dynamic_config::StorageMock;
  using StorageMockPtr = std::unique_ptr<StorageMock>;
  using MiddlewareFactories = userver::ugrpc::client::MiddlewareFactories;
  using CompletionQueuePoolBase = userver::ugrpc::impl::CompletionQueuePoolBase;
  using CompletionQueuePoolBasePtr = std::shared_ptr<CompletionQueuePoolBase>;
  using ClientFactorySettings =  userver::ugrpc::client::ClientFactorySettings;

public:
  template <typename Client>
  std::unique_ptr<Client> make_client(
    const std::string& client_name,
    const std::string& endpoint)
  {
    return std::make_unique<Client>(
      client_factory_->MakeClient<Client>(
        client_name, endpoint));
  }

protected:
  ~GrpcClientFactory() override = default;

private:
  explicit GrpcClientFactory(
    ClientFactoryConfig&& config,
    TaskProcessor& channel_task_processor,
    StatisticsStorage& statistics_storage,
    const CompletionQueuePoolBasePtr& queue_pool,
    const MiddlewareFactories& middleware_factories = {});

private:
  friend class UServerUtils::ComponentsBuilder;

  GrpcStatisticsStorage grpc_statistics_storage_;

  RegistratorDynamicSettings registrator_dynamic_settings_;

  GrpcControl testsuite_grpc_;

  StorageMockPtr storage_mock_;

  ClientFactoryPtr client_factory_;
};

using GrpcClientFactory_var = ReferenceCounting::SmartPtr<GrpcClientFactory>;

} // namespace UServerUtils::UServerGrpc

#endif // USERVER_USERVERGRPC_CLIENTFACTORY_HPP
