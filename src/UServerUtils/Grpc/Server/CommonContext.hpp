#ifndef GRPC_SERVER_COMMON_CONTEXT_H_
#define GRPC_SERVER_COMMON_CONTEXT_H_

// THIS
#include <Generics/CompositeActiveObject.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>

namespace UServerUtils::Grpc::Server
{

class CommonContext : public Generics::CompositeActiveObject
{
protected:
  CommonContext() = default;

  virtual ~CommonContext() = default;
};

using CommonContext_var = ReferenceCounting::SmartPtr<CommonContext>;

} // namespace UServerUtils::Grpc::Server

#endif // GRPC_SERVER_COMMON_CONTEXT_H_
