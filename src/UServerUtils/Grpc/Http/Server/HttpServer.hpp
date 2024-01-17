#ifndef USERVER_HTTP_SERVER_HTTPSERVER_HPP
#define USERVER_HTTP_SERVER_HTTPSERVER_HPP

// STD
#include <mutex>
#include <memory>
#include <condition_variable>

// USERVER
#include <userver/dynamic_config/storage_mock.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/server/server.hpp>
#include <userver/utils/statistics/metrics_storage.hpp>

// THIS
#include <Logger/Logger.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <UServerUtils/Grpc/Component.hpp>
#include <UServerUtils/Grpc/Http/Server/Config.hpp>
#include <UServerUtils/Grpc/Http/Server/HttpHandler.hpp>

namespace UServerUtils::Http::Server
{

class HttpServer final
  : public UServerUtils::Grpc::Component,
    public ReferenceCounting::AtomicImpl
{
public:
  using Exception = ActiveObject::Exception;
  using AlreadyActive = ActiveObject::AlreadyActive;
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using Server = userver::server::Server;
  using ServerPtr = std::unique_ptr<Server>;
  using StorageMock = userver::dynamic_config::StorageMock;
  using StorageMockPtr = std::shared_ptr<StorageMock>;
  using StatisticsStorage = userver::utils::statistics::Storage;
  using MetricsStorage = userver::utils::statistics::MetricsStorage;
  using MetricsStoragePtr = std::shared_ptr<MetricsStorage>;
  using StatisticsEntry = userver::utils::statistics::Entry;
  using StatisticsEntries = std::vector<StatisticsEntry>;
  using TaskProcessor = userver::engine::TaskProcessor;
  using ServerConfig = UServerUtils::Http::Server::ServerConfig;
  using DynamicConfigSource = userver::dynamic_config::Source;

public:
  void activate_object() override;

  void deactivate_object() override;

  void wait_object() override;

  bool active() override;

protected:
  ~HttpServer() override;

private:
  explicit HttpServer(
    Logger* logger,
    const ServerConfig& config,
    TaskProcessor& listener_task_processor,
    StatisticsStorage& statistics_storage,
    const StorageMockPtr& storage_mock);

  void add_handler(
    internal::HttpHandlerImpl* http_handler,
    TaskProcessor& task_processor);

private:
  friend class HttpServerBuilder;

  StorageMockPtr storage_mock_;

  MetricsStoragePtr metrics_storage_;

  StatisticsEntries metrics_storage_registration_;

  Logger_var logger_;

  ServerPtr server_;

  ACTIVE_STATE state_ = AS_NOT_ACTIVE;

  std::mutex state_mutex_;

  std::condition_variable condition_variable_;
};

using HttpServer_var = ReferenceCounting::SmartPtr<HttpServer>;

} // namespace UServerUtils::Http::Server

#endif //USERVER_HTTP_SERVER_HTTPSERVER_HPP
