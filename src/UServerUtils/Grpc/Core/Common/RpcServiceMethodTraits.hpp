#ifndef GRPC_CORE_COMMON_RPC_SERVICE_METHOD_TRAITS_H_
#define GRPC_CORE_COMMON_RPC_SERVICE_METHOD_TRAITS_H_

// STD
#include <type_traits>

// THIS
#include <UServerUtils/Grpc/Core/Common/TypeTraits.hpp>

// PROTOBUF
#include <google/protobuf/message.h>

namespace UServerUtils::Grpc::Core::Common
{

namespace internal
{

template<class T, class = void>
struct has_method_name : std::false_type
{
};

template<class T>
struct has_method_name<T,
  std::void_t<std::enable_if_t<
    std::is_same_v<decltype(T::method_name), const char *()>>>> : std::true_type
{
};

template<class T, class = void>
struct has_incomingType : std::false_type
{
};

template<class T>
struct has_incomingType<T,
  std::void_t <typename T::IncomingType>> : std::true_type
{
};

template<class T, class = void>
struct has_outgoingType : std::false_type
{
};

template<class T>
struct has_outgoingType<T,
  std::void_t<typename T::OutgoingType>> : std::true_type
{
};

} // namespace internal

template<class RpcServiceMethodConcept>
struct RpcServiceMethodTraits
{
  static_assert(
    internal::has_method_name<RpcServiceMethodConcept>::value,
    "The RPC service method concept must provide a static member "
    "'const char* method_name()'.");

  static_assert(
    internal::has_incomingType<RpcServiceMethodConcept>::value,
    "The RPC service method concept must provide an IncomingType.");

  static_assert(
    internal::has_outgoingType<RpcServiceMethodConcept>::value,
    "The RPC service method concept must provide an OutgoingType.");

  static constexpr const char* method_name()
  {
    return RpcServiceMethodConcept::method_name();
  }

  using Request =
    StripStream<typename RpcServiceMethodConcept::IncomingType>;

  using Response =
    StripStream<typename RpcServiceMethodConcept::OutgoingType>;

  static_assert(
    std::is_base_of<google::protobuf::Message, Request>::value,
    "The RPC request type must be derived from google::protobuf::Message.");

  static_assert(
    std::is_base_of<google::protobuf::Message, Response>::value,
    "The RPC response type must be derived from google::protobuf::Message");

  static constexpr auto rpc_type =
    RpcType<typename RpcServiceMethodConcept::IncomingType,
            typename RpcServiceMethodConcept::OutgoingType>::value;
};

} // namespace UServerUtils::Grpc::Core::Common

#endif // GRPC_CORE_COMMON_RPC_SERVICE_METHOD_TRAITS_H_
