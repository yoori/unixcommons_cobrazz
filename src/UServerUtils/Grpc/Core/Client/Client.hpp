#ifndef GRPC_CORE_CLIENT_CLIENT_H_
#define GRPC_CORE_CLIENT_CLIENT_H_

// STD
#include <future>
#include <memory>

namespace UServerUtils::Grpc::Core::Client
{

using ClientId = std::uint64_t;

template<class Request>
class Client
{
public:
  using RequestPtr = std::unique_ptr<Request>;

public:
  virtual void start() noexcept = 0;

  virtual bool write(RequestPtr&& request) noexcept = 0;

  virtual bool writes_done() noexcept = 0;

  virtual ClientId get_id() const noexcept = 0;

  virtual bool stop() noexcept = 0;

protected:
  Client() = default;

  virtual ~Client() = default;
};

} // namespace UServerUtils::Grpc::Core::Client

#endif // GRPC_CORE_CLIENT_CLIENT_H_
