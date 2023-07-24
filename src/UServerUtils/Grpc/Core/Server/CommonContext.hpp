#ifndef GRPC_CORE_SERVER_COMMON_CONTEXT_H_
#define GRPC_CORE_SERVER_COMMON_CONTEXT_H_

// THIS
#include <Generics/CompositeActiveObject.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>

namespace UServerUtils::Grpc::Core::Server
{

class CommonContext : public Generics::CompositeActiveObject
{
protected:
  CommonContext() = default;

  virtual ~CommonContext() = default;
};

using CommonContext_var = ReferenceCounting::SmartPtr<CommonContext>;

} // namespace UServerUtils::Grpc::Core::Server

#endif // GRPC_CORE_SERVER_COMMON_CONTEXT_H_
