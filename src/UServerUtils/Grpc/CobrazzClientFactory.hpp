#ifndef USERVER_GRPC_COBRAZZPOOLCLIENTFACTORY_HPP
#define USERVER_GRPC_COBRAZZPOOLCLIENTFACTORY_HPP

// STD
#include <memory>

// USERVER
#include <engine/task/task_processor.hpp>

// THIS
#include <eh/Exception.hpp>
#include <Generics/CompositeActiveObject.hpp>
#include <Logger/Logger.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <UServerUtils/Grpc/Component.hpp>
#include <UServerUtils/Grpc/Core/Client/ConfigPoolCoro.hpp>

namespace UServerUtils::Grpc
{

class GrpcCobrazzPoolClientFactory final
{
public:
  using Logger_var = Logging::Logger_var;
  using TaskProcessor = userver::engine::TaskProcessor;
  using ConfigPoolCoro = UServerUtils::Grpc::Core::Client::ConfigPoolCoro;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  GrpcCobrazzPoolClientFactory() = default;

  ~GrpcCobrazzPoolClientFactory() = default;

  // You must ensure that ClientPool is destroyed
  // before call Manager::deactivate_object
  template<class ClientPool>
  static std::shared_ptr<ClientPool> create(
    const Logger_var& logger,
    const ConfigPoolCoro& config_pool,
    TaskProcessor& task_processor)
  {
    return ClientPool::create(
      logger,
      config_pool,
      task_processor);
  }

  template<class ClientPool>
  static std::shared_ptr<ClientPool> create(
    const Logger_var& logger,
    const ConfigPoolCoro& config_pool)
  {
    auto* current_task_processor =
      userver::engine::current_task::GetTaskProcessorOptional();
    if (!current_task_processor)
    {
      Stream::Error stream;
      stream << FNS
             << ": GrpcCobrazzPoolClientFactory::create"
                " must be call from coroutine pool";
      throw Exception(stream);
    }

    return ClientPool::create(
      logger,
      config_pool,
      *current_task_processor);
  }
};

} // namespace UServerUtils::Grpc

#endif // USERVER_GRPC_COBRAZZPOOLCLIENTFACTORY_HPP
