#ifndef USERVER_GRPC_SERVICE_HPP
#define USERVER_GRPC_SERVICE_HPP

// USERVER
#include <userver/ugrpc/server/server.hpp>

// THIS
#include <Generics/ActiveObject.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include "Component.hpp"

namespace UServerUtils
{
namespace Grpc
{

class GrpcServiceBase :
  public Component,
  public userver::ugrpc::server::ServiceBase
{
protected:
  GrpcServiceBase() = default;

  ~GrpcServiceBase() override = default;
};

using GrpcServiceBase_var = ReferenceCounting::SmartPtr<GrpcServiceBase>;

} // namespace Grpc
} //namespace UServerUtils

#endif //USERVER_GRPC_SERVICE_HPP
