#ifndef USERVER_GRPC_SERVERBUILDER_HPP
#define USERVER_GRPC_SERVERBUILDER_HPP

// STD
#include <deque>
#include <memory>

// GRCCPP
#include <grpcpp/completion_queue.h>

// USERVER
#include <userver/engine/task/task_processor.hpp>
#include <userver/utils/statistics/storage.hpp>

// THIS
#include <eh/Exception.hpp>
#include <UServerUtils/Grpc/RegistratorDynamicSettings.hpp>
#include <UServerUtils/Grpc/Server.hpp>
#include <UServerUtils/Grpc/ServiceBase.hpp>

namespace UServerUtils::Grpc
{

class GrpcServerBuilder final
  : protected Generics::Uncopyable
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using TaskProcessor = userver::engine::TaskProcessor;
  using StatisticsStorage = GrpcServer::StatisticsStorage;
  using CompletionQueue = grpc::CompletionQueue;
  using GrpcServices = std::deque<GrpcServiceBase_var>;
  using Middlewares = userver::ugrpc::server::Middlewares;
  using MiddlewaresPtr = std::unique_ptr<Middlewares>;
  using MiddlewaresList = std::list<MiddlewaresPtr>;
  using StorageMock = userver::dynamic_config::StorageMock;
  using StorageMockPtr = std::unique_ptr<StorageMock>;
  using RegistratorDynamicSettings = UServerUtils::Grpc::RegistratorDynamicSettings;
  using RegistratorDynamicSettingsPtr = std::unique_ptr<RegistratorDynamicSettings>;

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
    MiddlewaresList middlewares_list;
  };

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  explicit GrpcServerBuilder(
    Logger* logger,
    GrpcServerConfig&& config,
    StatisticsStorage& statistics_storage);

  ~GrpcServerBuilder() = default;

  CompletionQueue& get_completion_queue();

  void add_grpc_service(
    TaskProcessor& task_processor,
    GrpcServiceBase* service,
    const Middlewares& middlewares = {});

private:
  ServerInfo build();

private:
  friend class ComponentsBuilder;

  RegistratorDynamicSettings registrator_dynamic_settings_;

  StorageMockPtr storage_mock_;

  GrpcServer_var grpc_server_;

  GrpcServices services_;

  MiddlewaresList middlewares_list_;
};

using GrpcServerBuilderPtr = std::unique_ptr<GrpcServerBuilder>;

} // namespace UServerUtils::Grpc

#endif //UNIXCOMMONS_COBRAZZ_SERVERBUILDER_HPP