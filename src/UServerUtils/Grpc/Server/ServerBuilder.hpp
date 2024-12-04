#ifndef GRPC_SERVER_SERVER_SERVERBUILDER_H_
#define GRPC_SERVER_SERVER_SERVERBUILDER_H_

// STD
#include <deque>
#include <memory>

// USERVER
#include <eh/Exception.hpp>
#include <userver/engine/task/task_processor.hpp>

// THIS
#include <Logger/Logger.hpp>
#include <UServerUtils/Grpc/Server/ConfigCoro.hpp>
#include <UServerUtils/Grpc/Server/ServerCoro.hpp>
#include <UServerUtils/Grpc/Server/ServiceCoro.hpp>

namespace UServerUtils
{

class ComponentsBuilder;

} // namespace UServerUtils

namespace UServerUtils::Grpc::Server
{

class ServerBuilder final : protected Generics::Uncopyable
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using TaskProcessor = userver::engine::TaskProcessor;
  using Config = Grpc::Server::ConfigCoro;
  using GrpcServer_var = Grpc::Server::ServerCoro_var;
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
  explicit ServerBuilder(
    const Config& config,
    Logger* logger);

  ~ServerBuilder() = default;

  const Common::SchedulerPtr&
  scheduler() const noexcept;

  template<class Service>
  void add_service(
    Service* service,
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
    services_.emplace_back(
      Component_var(ReferenceCounting::add_ref(service)));
  }

private:
  ServerInfo build();

private:
  friend class UServerUtils::ComponentsBuilder;

  GrpcServer_var grpc_server_;

  GrpcServices services_;
};

using ServerBuilderPtr = std::unique_ptr<ServerBuilder>;

} // namespace UServerUtils::Grpc::Server


#endif // GRPC_SERVER_SERVER_SERVERBUILDER_H_