#ifndef GRPC_CLIENT_CLIENT_OBSERVER_H_
#define GRPC_CLIENT_CLIENT_OBSERVER_H_

// STD
#include <memory>

// GRPC
#include <grpcpp/grpcpp.h>

// THIS
#include <Generics/Uncopyable.hpp>
#include <UServerUtils/Grpc/Client/Writer.hpp>
#include <UServerUtils/Grpc/Client/Types.hpp>
#include <UServerUtils/Grpc/Common/TypeTraits.hpp>

namespace UServerUtils::Grpc::Client
{

template<class RpcServiceMethodConcept>
class ClientImpl;

namespace Internal
{

template<class RpcServiceMethodConcept>
class FactoryImpl;

} // namespace Internal

template<class RpcServiceMethodConcept, class = void>
class ClientObserver
{
};

template<class RpcServiceMethodConcept>
class ClientObserver<
  RpcServiceMethodConcept,
  Internal::has_bidi_streaming_t<RpcServiceMethodConcept>>
  : protected Generics::Uncopyable
{
public:
  using Traits = UServerUtils::Grpc::Common::RpcServiceMethodTraits<
    RpcServiceMethodConcept>;
  using Request = typename Traits::Request;
  using Response = typename Traits::Response;

  static constexpr auto k_rpc_type = Traits::rpc_type;
  using WriterPtr = std::shared_ptr<Writer<Request, k_rpc_type>>;
  using CompletionQueue = grpc::CompletionQueue;
  using CompletionQueuePtr = std::shared_ptr<CompletionQueue>;
  using ClientId = UServerUtils::Grpc::Client::ClientId;

public:
  virtual void on_initialize(const bool ok) = 0;

  virtual void on_read(Response&& response) = 0;

  virtual void on_finish(grpc::Status&& status) = 0;

protected:
  ClientObserver() = default;

  virtual ~ClientObserver() = default;

  ClientId client_id() const noexcept
  {
    return client_id_;
  }

  const WriterPtr& writer() const noexcept
  {
    return writer_;
  }

  const CompletionQueuePtr& completion_queue() const noexcept
  {
    return completion_queue_;
  }

private:
  void set_client_id(const ClientId client_id) noexcept
  {
    client_id_ = client_id;
  }

  void set_writer(const WriterPtr& writer)
  {
    writer_ = writer;
  }

  void set_completion_queue(const CompletionQueuePtr& completion_queue)
  {
    completion_queue_ = completion_queue;
  }

private:
  friend class ClientImpl<RpcServiceMethodConcept>;
  friend class Internal::FactoryImpl<RpcServiceMethodConcept>;

  ClientId client_id_ = 0;

  WriterPtr writer_;

  CompletionQueuePtr completion_queue_;
};

template<class RpcServiceMethodConcept>
class ClientObserver<
  RpcServiceMethodConcept,
  Internal::has_server_streaming_t<RpcServiceMethodConcept>>
  : protected Generics::Uncopyable
{
public:
  using Traits = UServerUtils::Grpc::Common::RpcServiceMethodTraits<
    RpcServiceMethodConcept>;
  using Request = typename Traits::Request;
  using Response = typename Traits::Response;

  static constexpr auto k_rpc_type = Traits::rpc_type;

public:
  virtual void on_initialize(const bool ok) = 0;

  virtual void on_read(Response&& response) = 0;

  virtual void on_finish(grpc::Status&& status) = 0;

protected:
  ClientObserver() = default;

  virtual ~ClientObserver() = default;
};

template<class RpcServiceMethodConcept>
class ClientObserver<
  RpcServiceMethodConcept,
  Internal::has_client_streaming_t<RpcServiceMethodConcept>>
  : protected Generics::Uncopyable
{
public:
  using Traits = UServerUtils::Grpc::Common::RpcServiceMethodTraits<
    RpcServiceMethodConcept>;
  using Request = typename Traits::Request;
  using Response = typename Traits::Response;

  static constexpr auto k_rpc_type = Traits::rpc_type;

  using WriterPtr = std::shared_ptr<Writer<Request, k_rpc_type>>;
  using CompletionQueue = grpc::CompletionQueue;
  using CompletionQueuePtr = std::shared_ptr<CompletionQueue>;

public:
  virtual void on_initialize(const bool ok) = 0;

  virtual void on_finish(
    grpc::Status&& status,
    Response&& response) = 0;

protected:
  ClientObserver() = default;

  virtual ~ClientObserver() = default;

  ClientId client_id() const noexcept
  {
    return client_id_;
  }

  const WriterPtr& writer() const noexcept
  {
    return writer_;
  }

  const CompletionQueuePtr& completion_queue() const noexcept
  {
    return completion_queue_;
  }

private:
  void set_client_id(const ClientId client_id) noexcept
  {
    client_id_ = client_id;
  }

  void set_writer(const WriterPtr& writer) noexcept
  {
    writer_ = writer;
  }

  void set_completion_queue(const CompletionQueuePtr& completion_queue)
  {
    completion_queue_ = completion_queue;
  }

private:
  friend class ClientImpl<RpcServiceMethodConcept>;
  friend class Internal::FactoryImpl<RpcServiceMethodConcept>;

  ClientId client_id_ = 0;

  WriterPtr writer_;

  CompletionQueuePtr completion_queue_;
};

template<class RpcServiceMethodConcept>
class ClientObserver<
  RpcServiceMethodConcept,
  Internal::has_normal_rpc_t<RpcServiceMethodConcept>>
  : protected Generics::Uncopyable
{
public:
  using Traits = UServerUtils::Grpc::Common::RpcServiceMethodTraits<
    RpcServiceMethodConcept>;
  using Request = typename Traits::Request;
  using Response = typename Traits::Response;

  static constexpr auto k_rpc_type = Traits::rpc_type;
  using WriterPtr = std::unique_ptr<Writer<Request, k_rpc_type>>;

public:
  virtual void on_finish(
    grpc::Status&& status,
    Response&& response) = 0;

protected:
  ClientObserver() = default;

  virtual ~ClientObserver() = default;
};

} // namespace UServerUtils::Grpc::Client

#endif // GRPC_CLIENT_CLIENT_OBSERVER_H_