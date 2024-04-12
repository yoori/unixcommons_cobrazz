#ifndef GRPC_CORE_CLIENT_WRITER_H_
#define GRPC_CORE_CLIENT_WRITER_H_

// STD
#include <memory>

// PROTO
#include <grpcpp/grpcpp.h>

// THIS
#include <UServerUtils/Grpc/Core/Client/Client.hpp>
#include <UServerUtils/Grpc/Core/Client/Types.hpp>

namespace UServerUtils::Grpc::Core::Client
{

enum class WriterStatus
{
  Ok = 0,
  RpcClosed,
  InternalError
};

template<class Request, Internal::RpcType rpcType>
class Writer
{
};

template<class Request>
class Writer<Request, Internal::RpcType::NORMAL_RPC>
{
public:
  using ClientPtr = std::weak_ptr<Client<Request>>;

public:
  explicit Writer(ClientPtr&& client)
    : client_(std::move(client))
  {
  }

  virtual ~Writer() = default;

private:
  ClientPtr client_;
};

template<class Request>
class Writer<Request, Internal::RpcType::SERVER_STREAMING>
  : public Writer<Request, Internal::RpcType::NORMAL_RPC>
{
};

template<class Request>
class Writer<Request, Internal::RpcType::BIDI_STREAMING>
{
public:
  using RequestPtr = std::unique_ptr<Request>;
  using ClientPtr = std::weak_ptr<Client<Request>>;

public:
  explicit Writer(ClientPtr&& client)
    : client_(std::move(client))
  {
  }

  virtual ~Writer() = default;

  WriterStatus write(RequestPtr&& request) const noexcept
  {
    if (auto client = client_.lock())
    {
      if (client->write(std::move(request)))
      {
        return WriterStatus::Ok;
      }
      else
      {
        return WriterStatus::InternalError;
      }
    }
    else
    {
      return WriterStatus::RpcClosed;
    }
  }

  WriterStatus writes_done() const noexcept
  {
    if (auto client = client_.lock())
    {
      if (client->writes_done())
      {
        return WriterStatus::Ok;
      }
      else
      {
        return WriterStatus::InternalError;
      }
    }
    else
    {
      return WriterStatus::RpcClosed;
    }
  }

private:
  ClientPtr client_;
};

template<class Request>
class Writer<Request, Internal::RpcType::CLIENT_STREAMING>
  : public Writer<Request, Internal::RpcType::BIDI_STREAMING>
{
};

} // namespace UServerUtils::Grpc::Core::Client

#endif // GRPC_CORE_CLIENT_WRITER_H_