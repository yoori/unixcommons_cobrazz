// THIS
#include <UServerUtils/Grpc/Server/ServerCoro.hpp>

namespace UServerUtils::Grpc::Server
{

namespace Aspect
{

const char SERVER_CORO[] = "SERVER_CORO";

} // namespace Aspect

ServerCoro::ServerCoro(
  const ConfigCoro& config,
  Logger* logger)
  : logger_(ReferenceCounting::add_ref(logger)),
    common_context_coro_(new CommonContextCoro(
      logger,
      config.max_size_queue))
{
  Config config_server;
  config_server.ip = config.ip;
  config_server.port = config.port;
  config_server.num_threads = config.num_threads;
  config_server.channel_args = config.channel_args;
  config_server.common_context = common_context_coro_;
  config_server.request_handler_type = RequestHandlerType::Move;

  server_ = ReferenceCounting::SmartPtr<Server>(
    new Server(
      config_server,
      logger_));
}

const Common::SchedulerPtr& ServerCoro::scheduler() const noexcept
{
  return server_->scheduler();
}

ServerCoro::~ServerCoro()
{
  try
  {
    deactivate_object();
  }
  catch (const eh::Exception& exc)
  {
    std::cerr << FNS
              << "Exception: "
              << exc.what();
  }
  catch (...)
  {
    std::cerr << FNS
              << "Exception: Unknown error";
  }
}

void ServerCoro::activate_object_()
{
  common_context_coro_.reset();
  server_->activate_object();
}

void ServerCoro::deactivate_object_()
{
  server_->deactivate_object();
  server_->wait_object();
}

void ServerCoro::wait_object_()
{
  UServerUtils::Component::CompositeActiveObjectBase::SimpleActiveObject::wait_object_();
}

} // namespace UServerUtils::Grpc::Server
