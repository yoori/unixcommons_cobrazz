#ifndef USERVER_GRPC_SERVER_HPP
#define USERVER_GRPC_SERVER_HPP

// Std
#include <mutex>
#include <memory>
#include <condition_variable>

// USERVER
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/ugrpc/server/server.hpp>
#include <userver/utils/statistics/fwd.hpp>
#include <userver/utils/statistics/storage.hpp>

// THIS
#include <Generics/ActiveObject.hpp>
#include <Logger/ActiveObjectCallback.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include "Component.hpp"
#include "Config.hpp"

namespace UServerUtils
{
namespace Grpc
{

class GrpcServer final
  : public Component,
    public virtual ReferenceCounting::AtomicImpl
{
public:
  using Exception = ActiveObject::Exception;
  using AlreadyActive = ActiveObject::AlreadyActive;

  using ActiveObjectCallback = Generics::ActiveObjectCallback;
  using ActiveObjectCallback_var = Generics::ActiveObjectCallback_var;

  using Server = userver::ugrpc::server::Server;
  using ServerConfig = userver::ugrpc::server::ServerConfig;
  using TaskProcessor = userver::engine::TaskProcessor;
  using CompletionQueue = grpc::CompletionQueue;
  using StatisticsStorage = userver::utils::statistics::Storage;
  using Service = userver::ugrpc::server::ServiceBase;

public:
  ~GrpcServer() override;

  void activate_object() override;

  void deactivate_object() override;

  void wait_object() override;

  bool active() override;

  CompletionQueue& get_completion_queue() noexcept;

private:
  friend class GrpcServerBuilder;

  explicit GrpcServer(
    ServerConfig&& config,
    StatisticsStorage& statistics_storage,
    ActiveObjectCallback* callback = nullptr);

  void add_service(
    Service& service,
    TaskProcessor& task_processor);

private:
  Server server_;

  ActiveObjectCallback_var callback_ = nullptr;

  ACTIVE_STATE state_ = AS_NOT_ACTIVE;

  std::mutex state_mutex_;

  std::condition_variable condition_variable_;
};

using GrpcServer_var = ReferenceCounting::SmartPtr<GrpcServer>;

} // namespace Grpc
} // namespace UServerUtils

#endif //USERVER_GRPC_SERVER_HPP
