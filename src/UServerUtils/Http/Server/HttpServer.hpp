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
#include <UServerUtils/Http/Server/Config.hpp>
#include <UServerUtils/Http/Server/HttpHandler.hpp>
#include <UServerUtils/Component.hpp>

namespace UServerUtils::Http::Server
{

class HttpServer final
  : public UServerUtils::Component,
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

protected:
  ~HttpServer() override;

private:
  void activate_object_() override;

  void deactivate_object_() override;

  void wait_object_() override;

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
};

using HttpServer_var = ReferenceCounting::SmartPtr<HttpServer>;

} // namespace UServerUtils::Http::Server

#endif //USERVER_HTTP_SERVER_HTTPSERVER_HPP
