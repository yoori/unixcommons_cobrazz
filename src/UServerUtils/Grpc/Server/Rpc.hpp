#ifndef GRPC_SERVER_RPC_H_
#define GRPC_SERVER_RPC_H_

// STD
#include <memory>

// GRPC
#include <grpcpp/support/status.h>

// PROTOBUF
#include <google/protobuf/message.h>

// THIS
#include <Generics/Uncopyable.hpp>

namespace UServerUtils::Grpc::Server
{

class Rpc : protected Generics::Uncopyable
{
public:
  using Message = google::protobuf::Message;
  using MessagePtr = std::unique_ptr<Message>;

public:
  virtual bool write(MessagePtr&& message) noexcept = 0;

  virtual bool finish(grpc::Status&& status) noexcept = 0;

  virtual bool stop() noexcept = 0;

  virtual bool is_stopped() noexcept = 0;

  virtual std::weak_ptr<Rpc> get_weak_ptr() noexcept = 0;

protected:
  Rpc() = default;

  virtual ~Rpc() = default;
};

} // namespace UServerUtils::Grpc::Server

#endif // GRPC_SERVER_RPC_H_
