#ifndef GRPC_CLIENT_CLIENT_DELEGATE_H_
#define GRPC_CLIENT_CLIENT_DELEGATE_H_

// THIS
#include <UServerUtils/Grpc/Client/Client.hpp>

namespace UServerUtils::Grpc::Client
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

} // namespace UServerUtils::Grpc::Client

#endif // GRPC_CLIENT_CLIENT_DELEGATE_H_
