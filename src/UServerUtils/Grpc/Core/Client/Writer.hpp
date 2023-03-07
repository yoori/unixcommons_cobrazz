#ifndef GRPC_CORE_CLIENT_WRITER_H_
#define GRPC_CORE_CLIENT_WRITER_H_

// STD
#include <memory>

// PROTO
#include <grpcpp/grpcpp.h>

// THIS
#include "Client.hpp"
#include "Types.hpp"

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
  using CompletionQueue = grpc::CompletionQueue;
  using CompletionQueuePtr = std::shared_ptr<CompletionQueue>;

public:
  explicit Writer(
    const CompletionQueuePtr& completion_queue,
    ClientPtr&& client,
    const ClientId client_id)
    : completion_queue_(completion_queue),
      client_(std::move(client)),
      client_id_(client_id)
  {
  }

  virtual ~Writer() = default;

private:
  CompletionQueuePtr completion_queue_;

  ClientPtr client_;

  const ClientId client_id_;
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
  using CompletionQueue = grpc::CompletionQueue;
  using CompletionQueuePtr = std::shared_ptr<CompletionQueue>;

public:
  explicit Writer(
    const CompletionQueuePtr& completion_queue,
    ClientPtr&& client,
    const ClientId client_id)
    : completion_queue_(completion_queue),
      client_(std::move(client)),
      client_id_(client_id)
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

  ClientId client_id() const noexcept
  {
    return client_id_;
  }

  CompletionQueuePtr completion_queue() const noexcept
  {
    return completion_queue_;
  }

private:
  CompletionQueuePtr completion_queue_;

  ClientPtr client_;

  const ClientId client_id_;
};

template<class Request>
class Writer<Request, Internal::RpcType::CLIENT_STREAMING>
  : public Writer<Request, Internal::RpcType::BIDI_STREAMING>
{
};

} // namespace UServerUtils::Grpc::Core::Client

#endif // GRPC_CORE_CLIENT_WRITER_H_