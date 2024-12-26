#ifndef GRPC_SERVER_DEFAULT_ERROR_CREATOR_H_
#define GRPC_SERVER_DEFAULT_ERROR_CREATOR_H_

// THIS
#include <Generics/Uncopyable.hpp>

// STD
#include <memory>

namespace UServerUtils::Grpc::Server
{

template<class Response>
class DefaultErrorCreator : private Generics::Uncopyable
{
public:
  DefaultErrorCreator() = default;

  virtual ~DefaultErrorCreator() = default;

  virtual std::unique_ptr<Response> create() noexcept = 0;
};

template<class Response>
using DefaultErrorCreatorPtr = std::unique_ptr<DefaultErrorCreator<Response>>;

template<class Request, class Response>
class FactoryDefaultErrorCreator : private Generics::Uncopyable
{
public:
  FactoryDefaultErrorCreator() = default;

  virtual ~FactoryDefaultErrorCreator() = default;

  virtual DefaultErrorCreatorPtr<Response> create(
    const Request& /*request*/) noexcept = 0;
};

template<class Request, class Response>
using FactoryDefaultErrorCreatorPtr = std::unique_ptr<
  FactoryDefaultErrorCreator<Request, Response>>;

} // namespace UServerUtils::Grpc::Server

#endif //GRPC_SERVER_DEFAULT_ERROR_CREATOR_H_