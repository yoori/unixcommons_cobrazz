/// STD
#include <sstream>

// THIS
#include <UServerUtils/Grpc/Core/Common/Logging.hpp>
#include <UServerUtils/Grpc/Core/Common/Utils.hpp>
#include <UServerUtils/Grpc/Core/Server/Server.hpp>

namespace UServerUtils::Grpc::Core::Server
{

namespace Aspect
{

const char SERVER[] = "GRPC_SERVER";

} // namespace Aspect

Server::Server(
  const Config& config,
  Logger* logger)
  : config_(std::move(config)),
    logger_(ReferenceCounting::add_ref(logger)),
    rpc_pool_(new RpcPoolImpl(logger_.in()))
{
  namespace Logger = UServerUtils::Grpc::Core::Common::Logger;
  Logger::set_logger(logger_.in());

  if (!config_.num_threads)
  {
    const auto best_thread_number =
      std::thread::hardware_concurrency();
    if (best_thread_number == 0)
    {
      Stream::Error stream;
      stream << FNS
             << ": hardware_concurrency is failed";
      logger_->error(stream.str(), Aspect::SERVER);
    }
    config_.num_threads =
      best_thread_number ? best_thread_number : 128;
  }

  Common::Scheduler::Queues queues;
  server_completion_queues_.reserve(*config_.num_threads);
  for (std::size_t i = 1; i <= *config_.num_threads; ++i)
  {
    std::shared_ptr<grpc::ServerCompletionQueue>
      server_completion_queue(
        server_builder_.AddCompletionQueue());
    server_completion_queues_.emplace_back(
      server_completion_queue);
    queues.emplace_back(server_completion_queue);
  }

  scheduler_ = Common::SchedulerPtr(
    new Common::Scheduler(
      logger_.in(),
      std::move(queues)));
}

Server::~Server()
{
  try
  {
    Stream::Error stream;
    bool error = false;

    if (state_ == AS_ACTIVE)
    {
      stream << FNS
             << ": wasn't deactivated.";
      error = true;

      try
      {
        rpc_pool_->deactivate_object();
        rpc_pool_->wait_object();
      }
      catch (...)
      {
      }

      const auto deadline =
        std::chrono::system_clock::now() +
        std::chrono::milliseconds(300);

      server_->Shutdown(deadline);

      for (auto& service : services_)
      {
        try
        {
          service->deactivate_object();
          service->wait_object();
        }
        catch (...)
        {
        }
      }

      try
      {
        if (config_.common_context)
        {
          config_.common_context->deactivate_object();
          config_.common_context->wait_object();
        }
      }
      catch (...)
      {
      }
    }

    if (state_ != AS_NOT_ACTIVE)
    {
      if (error)
      {
        stream << std::endl;
      }
      stream << FNS
             << ": didn't wait for deactivation, still active.";
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
      std::cerr << FNS
                << ": eh::Exception: "
                << exc.what()
                << std::endl;
    }
    catch (...)
    {
    }
  }
}

void Server::activate_object()
{
  std::stringstream stream;
  stream << FNS
         << ": Starting server on ip="
         << config_.ip
         << " , port="
         << config_.port
         << "...";
  logger_->info(stream.str(), Aspect::SERVER);

  std::lock_guard lock(state_mutex_);
  if (state_ != AS_NOT_ACTIVE)
  {
    Stream::Error stream;
    stream << FNS
           << ": still active";
    throw ActiveObject::AlreadyActive(stream);
  }

  try
  {
    if (config_.common_context)
    {
      config_.common_context->activate_object();
    }

    add_channel_args();
    server_builder_.AddListeningPort(
      config_.ip + ":" + std::to_string(config_.port),
      grpc::InsecureServerCredentials());
    register_services();

    server_ = server_builder_.BuildAndStart();
    if (!server_)
    {
      Stream::Error stream;
      stream << FNS
             << ": buildAndStart is failed";
      throw ActiveObject::Exception(stream);
    }

    rpc_pool_->activate_object();

    for (auto& service : services_)
    {
      service->activate_object();
    }

    state_ = AS_ACTIVE;
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error stream;
    stream << FNS
           << ": activate_object failure: "
           << exc.what();
    throw Exception(stream);
  }

  logger_->info(
    std::string("Server is succesfully started"),
    Aspect::SERVER);
}

void Server::deactivate_object()
{
  std::unique_lock lock(state_mutex_);
  if (state_ == AS_ACTIVE)
  {
    logger_->info(
      std::string("Stopping Server..."),
      Aspect::SERVER);

    try
    {
      rpc_pool_->deactivate_object();
      rpc_pool_->wait_object();
    }
    catch (const eh::Exception& exc)
    {
      Stream::Error stream;
      stream << FNS
             << ": "
             << exc.what();
      logger_->error(stream.str(), Aspect::SERVER);
    }

    const auto deadline =
      std::chrono::system_clock::now() +
      std::chrono::milliseconds(100);

    server_->Shutdown(deadline);

    for (auto& service : services_)
    {
      try
      {
        service->deactivate_object();
        service->wait_object();
      }
      catch (const eh::Exception& exc)
      {
        Stream::Error stream;
        stream << FNS
               << ": "
               << exc.what();
        logger_->error(stream.str(), Aspect::SERVER);
      }
    }

    try
    {
      if (config_.common_context)
      {
        config_.common_context->deactivate_object();
        config_.common_context->wait_object();
      }
    }
    catch (const eh::Exception& exc)
    {
      Stream::Error stream;
      stream << FNS
             << ": "
             << exc.what();
      logger_->error(stream.str(), Aspect::SERVER);
    }

    state_ = AS_DEACTIVATING;
    lock.unlock();

    condition_variable_.notify_all();

    logger_->info(
      std::string("Server is succesfully stopped"),
      Aspect::SERVER);
  }
}

void Server::wait_object()
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

bool Server::active()
{
  std::lock_guard lock(state_mutex_);
  return state_ == AS_ACTIVE;
}

void Server::add_channel_args()
{
  namespace Utils = UServerUtils::Grpc::Core::Common::Utils;

  for (const auto& [name, value] : config_.channel_args)
  {
    if (Utils::is_integer(value))
    {
      server_builder_.AddChannelArgument(
        name,
        std::stoi(value));
    }
    else
    {
      server_builder_.AddChannelArgument(name, value);
    }
  }
}

void Server::register_services()
{
  for (auto& handler : handlers_)
  {
    Service::Handlers handlers;
    for (auto& [method_name, rpc_handler_info] : handler.second)
    {
      handlers.emplace_back(
        method_name,
        std::move(rpc_handler_info));
    }

    Service_var service(
      new Service(
        logger_.in(),
        rpc_pool_.in(),
        server_completion_queues_,
        config_.common_context,
        std::move(handlers)));
    server_builder_.RegisterService(service.in());
    services_.emplace_back(std::move(service));
  }
}

} // namespace UServerUtils::Grpc::Core::Server
