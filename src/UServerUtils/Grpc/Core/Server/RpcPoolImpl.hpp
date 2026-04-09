#pragma once

// STD
#include <mutex>
#include <condition_variable>
#include <unordered_map>

// THIS
#include <Generics/ActiveObject.hpp>
#include <Logger/Logger.hpp>
#include <UServerUtils/Grpc/Core/Server/RpcPool.hpp>

namespace UServerUtils::Grpc::Core::Server
{
  class RpcPoolImpl final:
    public RpcPool,
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
    const Logger_var logger_;

    mutable std::mutex mutex_;
    Rpcs rpcs_;
  };

  using RpcPoolImpl_var = ReferenceCounting::SmartPtr<RpcPoolImpl>;

} // namespace UServerUtils::Grpc::Core::Server
