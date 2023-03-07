#ifndef GRPC_CORE_COMMON_EVENT_H_
#define GRPC_CORE_COMMON_EVENT_H_

// THIS
#include <Generics/Uncopyable.hpp>

namespace UServerUtils::Grpc::Core::Common
{

class Event : protected Generics::Uncopyable
{
public:
  virtual void handle(const bool ok) noexcept = 0;

protected:
  Event() = default;

  virtual ~Event() = default;
};

} // namespace UServerUtils::Grpc::Core::Common

#endif // GRPC_CORE_COMMON_EVENT_H_
