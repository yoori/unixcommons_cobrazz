#ifndef GRPC_CORE_CLIENT_FACTORY_OBSERVER_H_
#define GRPC_CORE_CLIENT_FACTORY_OBSERVER_H_

// STD
#include <functional>

// THIS
#include <UServerUtils/Grpc/Core/Client/Client.hpp>

namespace UServerUtils::Grpc::Core::Client
{

using FactoryObserver = std::function<void(
  const ClientId client_id,
  const std::shared_ptr<grpc::Channel>& channel,
  const std::shared_ptr<grpc::CompletionQueue>& completion_queue)>;

} // namespace UServerUtils::Grpc::Core::Client

#endif // GRPC_CORE_CLIENT_FACTORY_OBSERVER_H_
