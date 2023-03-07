#ifndef GRPC_CORE_CLIENT_CLIENT_TYPES_H_
#define GRPC_CORE_CLIENT_CLIENT_TYPES_H_

// GRPC
#include <grpcpp/grpcpp.h>

// THIS
#include <UServerUtils/Grpc/Core/Common/RpcServiceMethodTraits.hpp>
#include <UServerUtils/Grpc/Core/Common/TypeTraits.hpp>

namespace UServerUtils::Grpc::Core::Client
{

namespace Internal
{

namespace Common = UServerUtils::Grpc::Core::Common;

using RpcType = grpc::internal::RpcMethod::RpcType;

template<class RpcServiceMethodConcept>
using Traits = Common::RpcServiceMethodTraits<
  RpcServiceMethodConcept>;

template<class RpcServiceMethodConcept>
constexpr auto rpc_type =
  Traits<RpcServiceMethodConcept>::rpc_type;

template<class RpcServiceMethodConcept>
using Request =
  typename Traits<RpcServiceMethodConcept>::Request;

template<class RpcServiceMethodConcept>
using Response =
  typename Traits<RpcServiceMethodConcept>::Response;

template<class T, class = void>
struct has_member_id_request_grpc : std::false_type {
};

template<class T>
struct has_member_id_request_grpc<
  T,
  std::void_t<
    std::enable_if_t<
      std::is_same_v<
        decltype(std::declval<T&>().id_request_grpc()),
        std::uint32_t>>>> : std::true_type {
};

template<class T>
constexpr bool has_member_id_request_grpc_v = has_member_id_request_grpc<T>::value;

template<class RpcServiceMethodConcept, class = void>
struct has_bidi_streaming
{
};

template<class RpcServiceMethodConcept>
struct has_bidi_streaming<
  RpcServiceMethodConcept,
  std::void_t<
    std::enable_if_t<
      Internal::rpc_type<RpcServiceMethodConcept> ==
      Internal::RpcType::BIDI_STREAMING>>>
{
  using Type = void;
};

template<class RpcServiceMethodConcept>
using has_bidi_streaming_t =
  typename has_bidi_streaming<RpcServiceMethodConcept>::Type;

template<class RpcServiceMethodConcept, class = void>
struct has_normal_rpc
{
};

template<class RpcServiceMethodConcept>
struct has_normal_rpc<
  RpcServiceMethodConcept,
  std::void_t<
    std::enable_if_t<
      Internal::rpc_type<RpcServiceMethodConcept> ==
      Internal::RpcType::NORMAL_RPC>>>
{
  using Type = void;
};

template<class RpcServiceMethodConcept>
using has_normal_rpc_t =
  typename has_normal_rpc<RpcServiceMethodConcept>::Type;

template<class RpcServiceMethodConcept, class = void>
struct has_client_streaming
{
};

template<class RpcServiceMethodConcept>
struct has_client_streaming<
  RpcServiceMethodConcept,
    std::void_t<
      std::enable_if_t<
        Internal::rpc_type<RpcServiceMethodConcept> ==
        Internal::RpcType::CLIENT_STREAMING>>>
{
  using Type = void;
};

template<class RpcServiceMethodConcept>
using has_client_streaming_t =
  typename has_client_streaming<RpcServiceMethodConcept>::Type;

template<class RpcServiceMethodConcept, class = void>
struct has_server_streaming
{
};

template<class RpcServiceMethodConcept>
struct has_server_streaming<
  RpcServiceMethodConcept,
  std::void_t<
    std::enable_if_t<
      Internal::rpc_type<RpcServiceMethodConcept> ==
      Internal::RpcType::SERVER_STREAMING>>>
{
  using Type = void;
};

template<class RpcServiceMethodConcept>
using has_server_streaming_t =
  typename has_server_streaming<RpcServiceMethodConcept>::Type;


} // namespace Internal

using IdRequest = std::uint32_t;

} // namespace UServerUtils::Grpc::Core::Client::Internal

#endif // GRPC_CORE_CLIENT_CLIENT_TYPES_H_
