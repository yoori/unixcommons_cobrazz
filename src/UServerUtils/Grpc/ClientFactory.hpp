#ifndef USERVER_GRPC_CLIENTFACTORY_HPP
#define USERVER_GRPC_CLIENTFACTORY_HPP

// STD
#include <memory>

// USERVER
#include <engine/task/task_processor.hpp>
#include <userver/ugrpc/client/client_factory.hpp>

// GRPCCPP
#include <grpcpp/completion_queue.h>

// THIS
#include <ReferenceCounting/AtomicImpl.hpp>
#include <UServerUtils/Grpc/Component.hpp>
#include <UServerUtils/Grpc/Config.hpp>

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

public:
  ~GrpcClientFactory() override = default;

  template <typename Client>
  std::unique_ptr<Client> make_client(const std::string& endpoint)
  {
    return std::make_unique<Client>(
      client_factory_->MakeClient<Client>(
        endpoint));
  }

private:
  explicit GrpcClientFactory(
    GrpcClientFactoryConfig&& config,
    TaskProcessor& channel_task_processor,
    CompletionQueue& queue,
    StatisticsStorage& statistics_storage);

private:
  friend class ComponentsBuilder;

  ClientFactoryPtr client_factory_;
};

using GrpcClientFactory_var = ReferenceCounting::SmartPtr<GrpcClientFactory>;

} // namespace UServerUtils::Grpc

#endif // USERVER_GRPC_CLIENTFACTORY_HPP
