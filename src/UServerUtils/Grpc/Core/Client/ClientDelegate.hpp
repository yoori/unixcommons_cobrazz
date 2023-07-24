#ifndef GRPC_CORE_CLIENT_CLIENT_DELEGATE_H_
#define GRPC_CORE_CLIENT_CLIENT_DELEGATE_H_

// THIS
#include <UServerUtils/Grpc/Core/Client/Client.hpp>

namespace UServerUtils::Grpc::Core::Client
{

template<class Request>
class ClientDelegate
{
public:
  using RequestPtr = std::unique_ptr<Request>;

public:
  virtual void need_remove(const ClientId id) noexcept = 0;

protected:
  ClientDelegate() = default;

  virtual ~ClientDelegate() = default;
};

} // namespace UServerUtils::Grpc::Core::Client

#endif // GRPC_CORE_CLIENT_CLIENT_DELEGATE_H_
