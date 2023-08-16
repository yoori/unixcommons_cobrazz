#ifndef USERVER_GRPC_SERVER_HPP
#define USERVER_GRPC_SERVER_HPP

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
#include <Logger/Logger.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <UServerUtils/Grpc/Component.hpp>
#include <UServerUtils/Grpc/Config.hpp>

namespace UServerUtils::Grpc
{

class GrpcServer final
  : public Component,
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
  using CompletionQueue = grpc::CompletionQueue;
  using StatisticsStorage = userver::utils::statistics::Storage;
  using Service = userver::ugrpc::server::ServiceBase;
  using Middlewares = userver::ugrpc::server::Middlewares;
  using StorageMock = userver::dynamic_config::StorageMock;
  using StorageMockPtr = std::unique_ptr<StorageMock>;

public:
  void activate_object() override;

  void deactivate_object() override;

  void wait_object() override;

  bool active() override;

  CompletionQueue& get_completion_queue() noexcept;

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
  const Logger_var logger_;

  StorageMockPtr storage_mock_;

  std::unique_ptr<Server> server_;

  ACTIVE_STATE state_ = AS_NOT_ACTIVE;

  std::mutex state_mutex_;

  std::condition_variable condition_variable_;
};

using GrpcServer_var = ReferenceCounting::SmartPtr<GrpcServer>;

} // namespace UServerUtils::Grpc

#endif //USERVER_GRPC_SERVER_HPP
