#ifndef SERVER_RPC_POOL_H_
#define SERVER_RPC_POOL_H_

// THIS
#include <Generics/ActiveObject.hpp>
#include <Logger/Logger.hpp>
#include "RpcPool.hpp"

// STD
#include <mutex>
#include <condition_variable>
#include <unordered_map>

namespace UServerUtils::Grpc::Core::Server
{

class RpcPoolImpl final
  : public RpcPool,
    public Generics::ActiveObject,
    public ReferenceCounting::AtomicImpl
{
public:
  using Logger_var = Logging::Logger_var;
  using RpcPtr = typename RpcPool::RpcPtr;
  using Rpcs = std::unordered_map<Rpc*, RpcPtr>;

public:
  RpcPoolImpl(const Logger_var& logger);

  ~RpcPoolImpl() override;

  void add(const RpcPtr& rpc) override;

  void remove(Rpc* rpc) noexcept override;

  void activate_object() override;

  void deactivate_object() override;

  void wait_object() override;

  bool active() override;

private:
  Logger_var logger_;

  Rpcs rpcs_;

  std::mutex mutex_;

  ACTIVE_STATE state_ = AS_NOT_ACTIVE;

  std::condition_variable condition_variable_;
};

using RpcPoolImpl_var = ReferenceCounting::SmartPtr<RpcPoolImpl>;

} // namespace UServerUtils::Grpc::Core::Server

#endif // SERVER_RPC_POOL_H_
