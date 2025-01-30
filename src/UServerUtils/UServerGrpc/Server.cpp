// THIS
#include <UServerUtils/Logger.hpp>
#include <UServerUtils/UServerGrpc/Server.hpp>

namespace UServerUtils::UServerGrpc
{

namespace Aspect
{

const char SERVER[] = "SERVER";

} // namespace Aspect

GrpcServer::GrpcServer(
  Logger* logger,
  ServerConfig&& config,
  StatisticsStorage& statistics_storage,
  StorageMockPtr&& storage_mock)
  : logger_(ReferenceCounting::add_ref(logger)),
    storage_mock_(std::move(storage_mock))
{
  server_ = std::make_unique<Server>(
    std::move(config),
    statistics_storage,
    storage_mock_->GetSource());
}

GrpcServer::~GrpcServer()
{
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

void GrpcServer::activate_object_()
{
  server_->Start();
}

void GrpcServer::deactivate_object_()
{
  server_->Stop();
}

void GrpcServer::add_service(
  Service& service,
  TaskProcessor& task_processor,
  const Middlewares& middlewares)
{
  if (active())
  {
    Stream::Error stream;
    stream << FNS
           << "Can't add service: GrpcServer is already active";
    throw Exception(stream.str());
  }

  server_->AddService(
    service,
    ServiceConfig{
      task_processor,
      middlewares});
}

} // namespace UServerUtils::UServerGrpc