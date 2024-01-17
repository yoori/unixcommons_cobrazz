#ifndef USERVER_HTTP_SERVER_HTTPSERVERBUILDER_HPP
#define USERVER_HTTP_SERVER_HTTPSERVERBUILDER_HPP

// STD
#include <deque>
#include <memory>

// USERVER
#include <userver/engine/task/task_processor.hpp>
#include <userver/utils/statistics/storage.hpp>
#include <userver/dynamic_config/storage_mock.hpp>

// THIS
#include <eh/Exception.hpp>
#include <Generics/Uncopyable.hpp>
#include <Logger/Logger.hpp>
#include <UServerUtils/Grpc/Http/Server/Config.hpp>
#include <UServerUtils/Grpc/Http/Server/HttpHandler.hpp>
#include <UServerUtils/Grpc/Http/Server/HttpServer.hpp>
#include <UServerUtils/Grpc/Component.hpp>
#include <UServerUtils/Grpc/RegistratorDynamicSettings.hpp>

namespace UServerUtils::Grpc
{

class ComponentsBuilder;

} // namespace UServerUtils::Grpc

namespace UServerUtils::Http::Server
{

class HttpServerBuilder final : protected Generics::Uncopyable
{
public:
  using ServerConfig = UServerUtils::Http::Server::ServerConfig;
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using StatisticsStorage = userver::utils::statistics::Storage;
  using StorageMock = userver::dynamic_config::StorageMock;
  using StorageMockPtr = std::shared_ptr<StorageMock>;
  using TaskProcessor = userver::engine::TaskProcessor;
  using Component_var = UServerUtils::Grpc::Component_var;
  using HttpHandlers = std::deque<UServerUtils::Grpc::Component_var>;
  using RegistratorDynamicSettings = UServerUtils::Grpc::RegistratorDynamicSettings;

  struct ServerInfo final
  {
    HttpServer_var http_server;
    HttpHandlers http_handlers;
  };

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  explicit HttpServerBuilder(
    Logger* logger,
    const ServerConfig& config,
    TaskProcessor& listener_task_processor,
    StatisticsStorage& statistics_storage);

  void add_handler(
    HttpHandler* http_handler,
    TaskProcessor& task_processor);

private:
  ServerInfo build();

private:
  friend class UServerUtils::Grpc::ComponentsBuilder;

  RegistratorDynamicSettings registrator_dynamic_settings_;

  StatisticsStorage& statistics_storage_;

  StorageMockPtr storage_mock_;

  HttpServer_var http_server_;

  HttpHandlers http_handlers_;
};

using HttpServerBuilderPtr = std::unique_ptr<HttpServerBuilder>;

} // namespace UServerUtils::Http::Server

#endif //USERVER_HTTP_SERVER_HTTPSERVERBUILDER_HPP