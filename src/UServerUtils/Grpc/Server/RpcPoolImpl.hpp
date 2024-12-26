#ifndef GRPC_SERVER_RPC_POOL_IMPL_H_
#define GRPC_SERVER_RPC_POOL_IMPL_H_

// THIS
#include <Generics/ActiveObject.hpp>
#include <Logger/Logger.hpp>
#include <UServerUtils/Grpc/Server/RpcPool.hpp>

// STD
#include <mutex>
#include <condition_variable>
#include <unordered_map>

namespace UServerUtils::Grpc::Server
{

class RpcPoolImpl final
  : public RpcPool,
    public Generics::SimpleActiveObject,
    public ReferenceCounting::AtomicImpl
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using RpcPtr = typename RpcPool::RpcPtr;
  using Rpcs = std::unordered_map<Rpc*, RpcPtr>;

public:
  RpcPoolImpl(Logger* logger);

  void add(const RpcPtr& rpc) override;

  void remove(Rpc* rpc) noexcept override;

protected:
  ~RpcPoolImpl() override;

  void deactivate_object_() override;

  void wait_object_() override;

private:
  Logger_var logger_;

  Rpcs rpcs_;

  mutable std::mutex mutex_;
};

using RpcPoolImpl_var = ReferenceCounting::SmartPtr<RpcPoolImpl>;

} // namespace UServerUtils::Grpc::Server

#endif // GRPC_SERVER_RPC_POOL_IMPL_H_
