// STD
#include <iostream>

// THIS
#include <UServerUtils/Grpc/Http/Server/HttpServer.hpp>

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
  server_config.monitor_listener = {};

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
    Stream::Error stream;
    bool error = false;

    if (state_ == AS_ACTIVE)
    {
      stream << FNS
             << "wasn't deactivated.";
      error = true;

      server_->Stop();
    }

    if (state_ != AS_NOT_ACTIVE)
    {
      if (error)
      {
        stream << std::endl;
      }
      stream << FNS << "didn't wait for deactivation, still active.";
      error = true;
    }

    if (error)
    {
      logger_->error(stream.str(), Aspect::HTTP_SERVER);
    }
  }
  catch (const eh::Exception& exc)
  {
    try
    {
      std::cerr << FNS << "eh::Exception: " << exc.what() << std::endl;
    }
    catch (...)
    {
    }
  }
}

void HttpServer::activate_object()
{
  std::lock_guard lock(state_mutex_);
  if (state_ != AS_NOT_ACTIVE)
  {
    Stream::Error stream;
    stream << FNS << "already active";
    throw ActiveObject::AlreadyActive(stream);
  }

  try
  {
    server_->Start();
    state_ = AS_ACTIVE;
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error stream;
    stream << FNS
           << "start failure: "
           << exc.what();
    throw Exception(stream);
  }
}

void HttpServer::deactivate_object()
{
  {
    std::lock_guard lock(state_mutex_);
    if (state_ == AS_ACTIVE)
    {
      server_->Stop();
      state_ = AS_DEACTIVATING;
    }
  }

  condition_variable_.notify_all();
}

void HttpServer::wait_object()
{
  std::unique_lock lock(state_mutex_);
  condition_variable_.wait(lock, [this] () {
    return state_ != AS_ACTIVE;
  });

  if (state_ == AS_DEACTIVATING)
  {
    state_ = AS_NOT_ACTIVE;
  }
}

bool HttpServer::active()
{
  std::lock_guard lock(state_mutex_);
  return state_ == AS_ACTIVE;
}

void HttpServer::add_handler(
  internal::HttpHandlerImpl* http_handler,
  TaskProcessor& task_processor)
{
  std::unique_lock lock(state_mutex_);
  if (state_ != AS_NOT_ACTIVE)
  {
    Stream::Error stream;
    stream << FNS
           << ": server is already activated";
    throw Exception(stream);
  }

  server_->AddHandler(*http_handler, task_processor);
}

} // namespace UServerUtils::Http