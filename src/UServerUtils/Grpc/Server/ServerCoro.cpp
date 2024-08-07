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
    common_context_coro_(new CommonContextCoro(logger, config.max_size_queue))
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

ServerCoro::~ServerCoro()
{
  try
  {
    Stream::Error stream;
    bool error = false;

    if (state_ == AS_ACTIVE)
    {
      stream << FNS
             << "wasn't deactivated.";
      error = true;

      server_->deactivate_object();
      server_->wait_object();
    }

    if (state_ != AS_NOT_ACTIVE)
    {
      if (error)
      {
        stream << std::endl;
      }
      stream << FNS
             << "didn't wait for deactivation, still active.";
      error = true;
    }

    if (error)
    {
      logger_->error(stream.str(), Aspect::SERVER_CORO);
    }
  }
  catch (const eh::Exception& exc)
  {
    try
    {
      std::cerr << FNS
                << "eh::Exception: "
                << exc.what()
                << std::endl;
    }
    catch (...)
    {
    }
  }
}

void ServerCoro::activate_object()
{
  common_context_coro_.reset();

  std::lock_guard lock(state_mutex_);
  if (state_ != AS_NOT_ACTIVE)
  {
    Stream::Error stream;
    stream << FNS << "already active";
    throw ActiveObject::AlreadyActive(stream);
  }

  try
  {
    server_->activate_object();
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

void ServerCoro::deactivate_object()
{
  {
    std::lock_guard lock(state_mutex_);
    if (state_ == AS_ACTIVE)
    {
      server_->deactivate_object();
      server_->wait_object();
      state_ = AS_DEACTIVATING;
    }
  }

  condition_variable_.notify_all();
}

void ServerCoro::wait_object()
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

bool ServerCoro::active()
{
  std::lock_guard lock(state_mutex_);
  return state_ == AS_ACTIVE;
}

} // namespace UServerUtils::Grpc::Server
