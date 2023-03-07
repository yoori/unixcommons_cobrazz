#ifndef USERVER_GRPC_COBRAZZ_SERVER_BUILDER_HPP
#define USERVER_GRPC_COBRAZZ_SERVER_BUILDER_HPP

// STD
#include <deque>
#include <memory>

// USERVER
#include <eh/Exception.hpp>
#include <engine/task/task_processor.hpp>

// THIS
#include "Core/Server/ConfigCoro.hpp"
#include "Core/Server/ServerCoro.hpp"
#include "Core/Server/ServiceCoro.hpp"
#include <Logger/Logger.hpp>

namespace UServerUtils::Grpc
{

class GrpcCobrazzServerBuilder final
  : protected Generics::Uncopyable
{
public:
  using Logger_var = Logging::Logger_var;
  using TaskProcessor = userver::engine::TaskProcessor;
  using Config = Core::Server::ConfigCoro;
  using GrpcServer_var = Core::Server::ServerCoro_var;
  using GrpcServices = std::deque<Component_var>;

  struct ServerInfo final
  {
    ServerInfo() = default;
    ~ServerInfo() = default;

    ServerInfo(const ServerInfo&) = delete;
    ServerInfo(ServerInfo&&) = default;
    ServerInfo& operator=(const ServerInfo&) = delete;
    ServerInfo& operator=(ServerInfo&&) = default;

    GrpcServer_var server;
    GrpcServices services;
  };

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  explicit GrpcCobrazzServerBuilder(
    const Config& config,
    const Logger_var& logger);

  ~GrpcCobrazzServerBuilder() = default;

  template<class Service>
  void add_service(
    const ReferenceCounting::SmartPtr<Service>& service,
    TaskProcessor& task_processor,
    std::optional<std::size_t> number_coro = {})
  {
    static_assert(
      std::is_base_of_v<Component, Service>,
      "Service must be derived from Componet");

    if (!grpc_server_)
    {
      Stream::Error stream;
      stream << FNS
             << ": grpc_server is null";
      throw Exception(stream);
    }

    grpc_server_->add_service(
      service,
      task_processor,
      number_coro);
    services_.emplace_back(service);
  }

private:
  ServerInfo build();

private:
  friend class ComponentsBuilder;

  GrpcServer_var grpc_server_;

  GrpcServices services_;
};

using GrpcCobrazzServerBuilderPtr =
  std::unique_ptr<GrpcCobrazzServerBuilder>;

} // namespace UServerUtils::Grpc


#endif // USERVER_GRPC_COBRAZZ_SERVER_BUILDER_HPP