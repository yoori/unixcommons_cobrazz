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
#include "Component.hpp"
#include "Config.hpp"

namespace UServerUtils
{
namespace Grpc
{

class GrpcClientFactory final :
  public Component,
  public virtual ReferenceCounting::AtomicImpl
{
  using ClientFactory = userver::ugrpc::client::ClientFactory;
  using ClientFactoryPtr = std::unique_ptr<ClientFactory>;
  using ClientFactoryConfig = userver::ugrpc::client::ClientFactoryConfig;
  using TaskProcessor = userver::engine::TaskProcessor;
  using CompletionQueue = grpc::CompletionQueue;
  using StatisticsStorage = userver::utils::statistics::Storage;

public:
  ~GrpcClientFactory() override;

  template <typename Client>
  std::unique_ptr<Client> make_client(const std::string& endpoint)
  {
    return std::make_unique<Client>(
      client_factory_->MakeClient<Client>(
        endpoint));
  }

private:
  GrpcClientFactory(
    GrpcClientFactoryConfig&& config,
    TaskProcessor& channel_task_processor,
    CompletionQueue& queue,
    StatisticsStorage& statistics_storage);

private:
  friend class ComponentsBuilder;

  ClientFactoryPtr client_factory_;
};

using GrpcClientFactory_var = ReferenceCounting::SmartPtr<GrpcClientFactory>;

} // namespace Grpc
} // namespace UServerUtils

#endif // USERVER_GRPC_CLIENTFACTORY_HPP
