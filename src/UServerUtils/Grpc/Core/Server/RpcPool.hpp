#pragma once

// STD
#include <memory>

// THIS
#include <ReferenceCounting/Interface.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <UServerUtils/Grpc/Core/Server/Rpc.hpp>

namespace UServerUtils::Grpc::Core::Server
{
  class RpcPool: public virtual ReferenceCounting::Interface
  {
  public:
    using RpcPtr = std::shared_ptr<Rpc>;

  public:
    virtual void add(const RpcPtr& rpc) = 0;

    virtual void remove(Rpc* rpc) noexcept = 0;
  };

  using RpcPool_var = ReferenceCounting::SmartPtr<RpcPool>;
} // namespace UServerUtils::Grpc::Core::Server
