#ifndef USERVER_GRPC_SERVERBUILDER_HPP
#define USERVER_GRPC_SERVERBUILDER_HPP

// STD
#include <deque>
#include <memory>

// GRCCPP
#include <grpcpp/completion_queue.h>

// USERVER
#include <eh/Exception.hpp>
#include <engine/task/task_processor.hpp>
#include <userver/utils/statistics/storage.hpp>

// THIS
#include "Server.hpp"
#include "ServiceBase.hpp"

namespace UServerUtils::Grpc
{

class GrpcServerBuilder final
  : protected Generics::Uncopyable
{
public:
  using Logger_var = Logging::Logger_var;
  using ActiveObjectCallback = Generics::ActiveObjectCallback;

  using TaskProcessor = userver::engine::TaskProcessor;
  using StatisticsStorage = GrpcServer::StatisticsStorage;

  using CompletionQueue = grpc::CompletionQueue;
  using GrpcServices = std::deque<GrpcServiceBase_var>;

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
  explicit GrpcServerBuilder(
    const Logger_var& logger,
    GrpcServerConfig&& config,
    StatisticsStorage& statistics_storage);

  ~GrpcServerBuilder() = default;

  CompletionQueue& get_completion_queue();

  void add_grpc_service(
    TaskProcessor& task_processor,
    GrpcServiceBase_var&& service);

private:
  ServerInfo build();

private:
  friend class ComponentsBuilder;

  GrpcServer_var grpc_server_;

  GrpcServices services_;
};

using GrpcServerBuilderPtr = std::unique_ptr<GrpcServerBuilder>;

} // namespace UServerUtils::Grpc

#endif //UNIXCOMMONS_COBRAZZ_SERVERBUILDER_HPP
