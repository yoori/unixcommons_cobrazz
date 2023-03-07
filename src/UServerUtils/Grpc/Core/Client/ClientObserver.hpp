#ifndef GRPC_CORE_CLIENT_CLIENT_OBSERVER_H_
#define GRPC_CORE_CLIENT_CLIENT_OBSERVER_H_

// STD
#include <memory>

// GRPC
#include <grpcpp/grpcpp.h>

// THIS
#include "Writer.hpp"
#include "Types.hpp"
#include "Common/TypeTraits.hpp"
#include <Generics/Uncopyable.hpp>

namespace UServerUtils::Grpc::Core::Client
{

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
  using Traits = UServerUtils::Grpc::Core::Common::RpcServiceMethodTraits<
    RpcServiceMethodConcept>;
  using Request = typename Traits::Request;
  using Response = typename Traits::Response;

  static constexpr auto k_rpc_type = Traits::rpc_type;
  using WriterPtr = std::unique_ptr<Writer<Request, k_rpc_type>>;

public:
  virtual void on_writer(WriterPtr&& writer) = 0;

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
  Internal::has_server_streaming_t<RpcServiceMethodConcept>>
  : protected Generics::Uncopyable
{
public:
  using Traits = UServerUtils::Grpc::Core::Common::RpcServiceMethodTraits<
    RpcServiceMethodConcept>;
  using Request = typename Traits::Request;
  using Response = typename Traits::Response;

  static constexpr auto k_rpc_type = Traits::rpc_type;
  using WriterPtr = std::unique_ptr<Writer<Request, k_rpc_type>>;

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
  using Traits = UServerUtils::Grpc::Core::Common::RpcServiceMethodTraits<
    RpcServiceMethodConcept>;
  using Request = typename Traits::Request;
  using Response = typename Traits::Response;

  static constexpr auto k_rpc_type = Traits::rpc_type;
  using WriterPtr = std::unique_ptr<Writer<Request, k_rpc_type>>;

public:
  virtual void on_writer(WriterPtr&& writer) = 0;

  virtual void on_initialize(const bool ok) = 0;

  virtual void on_finish(
    grpc::Status&& status,
    Response&& response) = 0;

protected:
  ClientObserver() = default;

  virtual ~ClientObserver() = default;
};

template<class RpcServiceMethodConcept>
class ClientObserver<
  RpcServiceMethodConcept,
  Internal::has_normal_rpc_t<RpcServiceMethodConcept>>
  : protected Generics::Uncopyable
{
public:
  using Traits = UServerUtils::Grpc::Core::Common::RpcServiceMethodTraits<
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

} // namespace UServerUtils::Grpc::Core::Client

#endif // GRPC_CORE_CLIENT_CLIENT_OBSERVER_H_