#include <UServerUtils/Grpc/Server.hpp>

namespace UServerUtils::Grpc
{

namespace Aspect
{

const char SERVER[] = "SERVER";

} // namespace Aspect

GrpcServer::GrpcServer(
  const Logger_var& logger,
  ServerConfig&& config,
  StatisticsStorage& statistics_storage)
  : logger_(logger),
    server_(std::move(config), statistics_storage)
{
}

GrpcServer::~GrpcServer()
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

      server_.Stop();
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
      logger_->error(stream.str(), Aspect::SERVER);
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

void GrpcServer::activate_object()
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
    server_.Start();
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

void GrpcServer::deactivate_object()
{
  {
    std::lock_guard lock(state_mutex_);
    if (state_ == AS_ACTIVE)
    {
      server_.Stop();
      state_ = AS_DEACTIVATING;
    }
  }

  condition_variable_.notify_all();
}

void GrpcServer::wait_object()
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

bool GrpcServer::active()
{
  std::lock_guard lock(state_mutex_);
  return state_ == AS_ACTIVE;
}

void GrpcServer::add_service(
  Service& service,
  TaskProcessor& task_processor)
{
  server_.AddService(service, task_processor);
}

GrpcServer::CompletionQueue& GrpcServer::get_completion_queue() noexcept
{
  return server_.GetCompletionQueue();
}

} // namespace UServerUtils::Grpc