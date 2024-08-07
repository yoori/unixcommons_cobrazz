#ifndef USERVER_GRPC_SERVICE_HPP
#define USERVER_GRPC_SERVICE_HPP

// USERVER
#include <userver/ugrpc/server/server.hpp>

// THIS
#include <UServerUtils/Component.hpp>

namespace UServerUtils::Grpc::Server
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

} // namespace UServerUtils::Grpc::Server

#endif //USERVER_GRPC_SERVICE_HPP
