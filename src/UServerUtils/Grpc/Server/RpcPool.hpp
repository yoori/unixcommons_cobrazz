#ifndef GRPC_SERVER_RPC_POOL_H_
#define GRPC_SERVER_RPC_POOL_H_

// STD
#include <memory>

// THIS
#include <ReferenceCounting/Interface.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <UServerUtils/Grpc/Server/Rpc.hpp>

namespace UServerUtils::Grpc::Server
{

class RpcPool : public virtual ReferenceCounting::Interface
{
public:
  using RpcPtr = std::shared_ptr<Rpc>;

public:
  virtual void add(const RpcPtr& rpc) = 0;

  virtual void remove(Rpc* rpc) noexcept = 0;

protected:
  RpcPool() = default;

  virtual ~RpcPool() = default;
};

using RpcPool_var = ReferenceCounting::SmartPtr<RpcPool>;

} // namespace UServerUtils::Grpc::Server

#endif // GRPC_SERVER_RPC_POOL_H_
