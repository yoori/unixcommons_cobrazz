// THIS
#include <UServerUtils/Grpc/Logger.hpp>
#include <UServerUtils/Grpc/Server.hpp>

namespace UServerUtils::Grpc
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
      std::make_shared<UServerUtils::Grpc::Logger::Logger>(logger),
      storage_mock_->GetSource());
  }

  GrpcServer::~GrpcServer()
  {
    try
    {
      if (active())
      {
        deactivate_object();
        wait_object();
      }
    }
    catch (const eh::Exception& exc)
    {
      std::cerr << FNS << "eh::Exception: " << exc.what() << std::endl;
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
    server_->AddService(service, task_processor, middlewares);
  }

  GrpcServer::CompletionQueue& GrpcServer::get_completion_queue() noexcept
  {
    return server_->GetCompletionQueue();
  }
} // namespace UServerUtils::Grpc
