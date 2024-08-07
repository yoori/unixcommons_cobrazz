#ifndef GRPC_COMMON_TYPE_TRAITS_H_
#define GRPC_COMMON_TYPE_TRAITS_H_

// STD
#include <type_traits>

// GRPC
#include <grpcpp/grpcpp.h>

namespace UServerUtils::Grpc::Common
{

template <typename Request>
struct Stream
{
  using type = Request;
};

template<template<class> class, class T>
struct Strip
{
  using type = T;
};

template<template<class> class T, class Param>
struct Strip<T, T<Param>>
{
  using type = Param;
};

template<class T>
using StripStream = typename Strip<Stream, T>::type;

template <typename Incoming, typename Outgoing>
struct RpcType
  : public std::integral_constant<grpc::internal::RpcMethod::RpcType,
                                  grpc::internal::RpcMethod::NORMAL_RPC>
{
};

template <typename Incoming, typename Outgoing>
struct RpcType<Stream<Incoming>, Outgoing>
  : public std::integral_constant<grpc::internal::RpcMethod::RpcType,
                                  grpc::internal::RpcMethod::CLIENT_STREAMING>
{
};

template <typename Incoming, typename Outgoing>
struct RpcType<Incoming, Stream<Outgoing>>
  : public std::integral_constant<grpc::internal::RpcMethod::RpcType,
                                  grpc::internal::RpcMethod::SERVER_STREAMING>
{
};

template <typename Incoming, typename Outgoing>
struct RpcType<Stream<Incoming>, Stream<Outgoing>>
  : public std::integral_constant<grpc::internal::RpcMethod::RpcType,
                                  grpc::internal::RpcMethod::BIDI_STREAMING>
{
};

} // namespace UServerUtils::Grpc::Common

#endif // GRPC_COMMON_THREAD_GUARD_H_
