#ifndef USERVER_GRPC_CLIENTFACTORY_HPP
#define USERVER_GRPC_CLIENTFACTORY_HPP

// STD
#include <memory>

// USERVER
#include <userver/engine/task/task_processor.hpp>
#include <userver/dynamic_config/storage_mock.hpp>
#include <userver/ugrpc/client/client_factory.hpp>

// GRPCCPP
#include <grpcpp/completion_queue.h>

// THIS
#include <ReferenceCounting/AtomicImpl.hpp>
#include <UServerUtils/Grpc/Component.hpp>
#include <UServerUtils/Grpc/Config.hpp>
#include <UServerUtils/Grpc/RegistratorDynamicSettings.hpp>

namespace UServerUtils::Grpc
{

class GrpcClientFactory final :
  public Component,
  public ReferenceCounting::AtomicImpl
{
  using ClientFactory = userver::ugrpc::client::ClientFactory;
  using ClientFactoryPtr = std::unique_ptr<ClientFactory>;
  using ClientFactoryConfig = userver::ugrpc::client::ClientFactoryConfig;
  using TaskProcessor = userver::engine::TaskProcessor;
  using CompletionQueue = grpc::CompletionQueue;
  using StatisticsStorage = userver::utils::statistics::Storage;
  using GrpcControl = userver::testsuite::GrpcControl;
  using StorageMock = userver::dynamic_config::StorageMock;
  using StorageMockPtr = std::unique_ptr<StorageMock>;
  using MiddlewareFactories = userver::ugrpc::client::MiddlewareFactories;

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
    GrpcClientFactoryConfig&& config,
    TaskProcessor& channel_task_processor,
    CompletionQueue& queue,
    StatisticsStorage& statistics_storage,
    const MiddlewareFactories& middleware_factories = {});

private:
  friend class ComponentsBuilder;

  RegistratorDynamicSettings registrator_dynamic_settings_;

  GrpcControl testsuite_grpc_;

  StorageMockPtr storage_mock_;

  ClientFactoryPtr client_factory_;
};

using GrpcClientFactory_var = ReferenceCounting::SmartPtr<GrpcClientFactory>;

} // namespace UServerUtils::Grpc

#endif // USERVER_GRPC_CLIENTFACTORY_HPP
