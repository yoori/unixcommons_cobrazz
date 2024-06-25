// STD
#include <iostream>

// THIS
#include <UServerUtils/Http/Server/HttpServer.hpp>

namespace UServerUtils::Http::Server
{

namespace Aspect
{

const char HTTP_SERVER[] = "HTTP_SERVER";

} // namespace Aspect

HttpServer::HttpServer(
  Logger* logger,
  const ServerConfig& config,
  userver::engine::TaskProcessor& listener_task_processor,
  StatisticsStorage& statistics_storage,
  const StorageMockPtr& storage_mock)
  : storage_mock_(storage_mock),
    metrics_storage_(std::make_shared<MetricsStorage>()),
    metrics_storage_registration_(metrics_storage_->RegisterIn(statistics_storage)),
    logger_(ReferenceCounting::add_ref(logger))
{
  userver::server::ServerConfig server_config;
  server_config.server_name = config.server_name;
  server_config.listener = config.listener_config;
  server_config.set_response_server_hostname = false;
  server_config.logger_access = {};
  server_config.logger_access_tskv = {};
  server_config.max_response_size_in_flight = {};
  server_config.monitor_listener = config.monitor_listener_config;

  server_ = std::make_unique<Server>(
    std::move(server_config),
    listener_task_processor,
    metrics_storage_,
    storage_mock->GetSource());
}

HttpServer::~HttpServer()
{
  for (auto& entry : metrics_storage_registration_)
  {
    entry.Unregister();
  }

  try
  {
    deactivate_object();
  }
  catch (const eh::Exception& exc)
  {
    std::cerr << FNS << "Exception: " << exc.what();
  }
  catch (...)
  {
    std::cerr << FNS << "Exception: Unknown error";
  }
}

void HttpServer::activate_object_()
{
  server_->Start();
}

void HttpServer::deactivate_object_()
{
  server_->Stop();
}

void HttpServer::wait_object_()
{
  UServerUtils::Component::CompositeActiveObjectBase::SimpleActiveObject::wait_object_();
}

void HttpServer::add_handler(
  internal::HttpHandlerImpl* http_handler,
  TaskProcessor& task_processor)
{
  if (active())
  {
    Stream::Error stream;
    stream << FNS
           << "Can't add service: HttpServer is already active";
    throw Exception(stream.str());
  }

  server_->AddHandler(*http_handler, task_processor);
}

} // namespace UServerUtils::Http