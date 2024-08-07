#ifndef GRPC_COMMON_EVENT_H_
#define GRPC_COMMON_EVENT_H_

// THIS
#include <Generics/Uncopyable.hpp>

namespace UServerUtils::Grpc::Common
{

class Event : protected Generics::Uncopyable
{
public:
  virtual void handle(const bool ok) noexcept = 0;

protected:
  Event() = default;

  virtual ~Event() = default;
};

} // namespace UServerUtils::Grpc::Common

#endif // GRPC_COMMON_EVENT_H_
