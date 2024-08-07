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
    public Generics::ActiveObject,
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

  void activate_object() override;

  void deactivate_object() override;

  void wait_object() override;

  bool active() override;

protected:
  ~RpcPoolImpl() override;

private:
  Logger_var logger_;

  Rpcs rpcs_;

  std::mutex mutex_;

  ACTIVE_STATE state_ = AS_NOT_ACTIVE;

  std::condition_variable condition_variable_;
};

using RpcPoolImpl_var = ReferenceCounting::SmartPtr<RpcPoolImpl>;

} // namespace UServerUtils::Grpc::Server

#endif // GRPC_SERVER_RPC_POOL_IMPL_H_
