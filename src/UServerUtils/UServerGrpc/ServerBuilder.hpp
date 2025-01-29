#ifndef USERVER_USERVERGRPC_SERVERBUILDER_HPP
#define USERVER_USERVERGRPC_SERVERBUILDER_HPP

// STD
#include <deque>
#include <memory>

// GRCCPP
#include <grpcpp/completion_queue.h>

// USERVER
#include <engine/task/task_processor.hpp>
#include <userver/utils/statistics/storage.hpp>

// THIS
#include <eh/Exception.hpp>
#include <UServerUtils/Grpc/Server/ServiceBase.hpp>
#include <UServerUtils/UServerGrpc/Config.hpp>
#include <UServerUtils/UServerGrpc/Server.hpp>
#include <UServerUtils/RegistratorDynamicSettings.hpp>


namespace UServerUtils
{

class ComponentsBuilder;

} // namespace UserverUtils

namespace UServerUtils::UServerGrpc
{

class GrpcServerBuilder final : protected Generics::Uncopyable
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using TaskProcessor = userver::engine::TaskProcessor;
  using StatisticsStorage = GrpcServer::StatisticsStorage;
  using GrpcServiceBase = UServerUtils::Grpc::Server::GrpcServiceBase;
  using GrpcServiceBase_var = UServerUtils::Grpc::Server::GrpcServiceBase_var;
  using GrpcServices = std::deque<GrpcServiceBase_var>;
  using Middlewares = userver::ugrpc::server::Middlewares;
  using MiddlewaresPtr = std::unique_ptr<Middlewares>;
  using MiddlewaresList = std::list<MiddlewaresPtr>;
  using StorageMock = userver::dynamic_config::StorageMock;
  using StorageMockPtr = std::unique_ptr<StorageMock>;
  using RegistratorDynamicSettings = UServerUtils::RegistratorDynamicSettings;

  struct ServerInfo final
  {
    GrpcServer_var server;
    GrpcServices services;
    MiddlewaresList middlewares_list;
  };

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  explicit GrpcServerBuilder(
    Logger* logger,
    ServerConfig&& config,
    StatisticsStorage& statistics_storage);

  ~GrpcServerBuilder() = default;

  void add_grpc_service(
    TaskProcessor& task_processor,
    GrpcServiceBase* service,
    const Middlewares& middlewares = {});

private:
  ServerInfo build();

private:
  friend class UServerUtils::ComponentsBuilder;

  RegistratorDynamicSettings registrator_dynamic_settings_;

  StorageMockPtr storage_mock_;

  GrpcServer_var grpc_server_;

  GrpcServices services_;

  MiddlewaresList middlewares_list_;
};

using GrpcServerBuilderPtr = std::unique_ptr<GrpcServerBuilder>;

} // namespace UServerUtils::UServerGrpc

#endif //USERVER_USERVERGRPC_SERVERBUILDER_HPP