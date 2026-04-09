#pragma once

// THIS
#include <Generics/ActiveObject.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>

namespace UServerUtils::Grpc::Core::Server
{
  class CommonContext: public virtual Generics::ActiveObject
  {
  protected:
    CommonContext() = default;

    virtual ~CommonContext() = default;
  };

  using CommonContext_var = ReferenceCounting::SmartPtr<CommonContext>;
} // namespace UServerUtils::Grpc::Core::Server
