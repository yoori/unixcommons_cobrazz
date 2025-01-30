#ifndef USERVER_USERVERGRPC_SERVER_HPP
#define USERVER_USERVERGRPC_SERVER_HPP

// Std
#include <mutex>
#include <memory>
#include <condition_variable>

// USERVER
#include <userver/dynamic_config/storage_mock.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/ugrpc/server/server.hpp>
#include <userver/utils/statistics/fwd.hpp>
#include <userver/utils/statistics/storage.hpp>

// THIS
#include <eh/Exception.hpp>
#include <Logger/Logger.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <UServerUtils/UServerGrpc/Config.hpp>
#include <UServerUtils/Component.hpp>

namespace UServerUtils::UServerGrpc
{

class GrpcServer final
  : public UServerUtils::Component,
    public ReferenceCounting::AtomicImpl
{
public:
  using Exception = ActiveObject::Exception;
  using AlreadyActive = ActiveObject::AlreadyActive;
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using Server = userver::ugrpc::server::Server;
  using ServerConfig = userver::ugrpc::server::ServerConfig;
  using TaskProcessor = userver::engine::TaskProcessor;
  using StatisticsStorage = userver::utils::statistics::Storage;
  using Service = userver::ugrpc::server::ServiceBase;
  using ServiceConfig = userver::ugrpc::server::ServiceConfig;
  using Middlewares = userver::ugrpc::server::Middlewares;
  using StorageMock = userver::dynamic_config::StorageMock;
  using StorageMockPtr = std::unique_ptr<StorageMock>;

protected:
  ~GrpcServer() override;

private:
  friend class GrpcServerBuilder;

  explicit GrpcServer(
    Logger* logger,
    ServerConfig&& config,
    StatisticsStorage& statistics_storage,
    StorageMockPtr&& storage_mock);

  void add_service(
    Service& service,
    TaskProcessor& task_processor,
    const Middlewares& middlewares);

private:
  void activate_object_() override;

  void deactivate_object_() override;

private:
  const Logger_var logger_;

  StorageMockPtr storage_mock_;

  std::unique_ptr<Server> server_;
};

using GrpcServer_var = ReferenceCounting::SmartPtr<GrpcServer>;

} // namespace UServerUtils::UServerGrpc

#endif //USERVER_USERVERGRPC_SERVER_HPP
