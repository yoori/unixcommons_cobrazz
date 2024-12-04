/// STD
#include <sstream>

// THIS
#include <UServerUtils/Grpc/Common/Logging.hpp>
#include <UServerUtils/Grpc/Common/Utils.hpp>
#include <UServerUtils/Grpc/Server/Server.hpp>

namespace UServerUtils::Grpc::Server
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
  namespace Logger = UServerUtils::Grpc::Common::Logger;
  Logger::set_logger(logger_.in());

  if (!config_.num_threads)
  {
    const auto best_thread_number = std::thread::hardware_concurrency();
    if (best_thread_number == 0)
    {
      Stream::Error stream;
      stream << FNS
             << "hardware_concurrency is failed";
      logger_->error(stream.str(), Aspect::SERVER);
    }
    config_.num_threads = best_thread_number ? best_thread_number : 128;
  }

  Common::Scheduler::Queues queues;
  server_completion_queues_.reserve(*config_.num_threads);
  for (std::size_t i = 1; i <= *config_.num_threads; ++i)
  {
    std::shared_ptr<grpc::ServerCompletionQueue> server_completion_queue(
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

const Common::SchedulerPtr& Server::scheduler() const noexcept
{
  return scheduler_;
}

Server::~Server()
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

void Server::activate_object_()
{
  std::stringstream stream;
  stream << FNS
         << "Starting server on ip="
         << config_.ip
         << " , port="
         << config_.port
         << "...";
  logger_->info(stream.str(), Aspect::SERVER);

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
             << "buildAndStart is failed";
      throw ActiveObject::Exception(stream);
    }

    rpc_pool_->activate_object();

    for (auto& service : services_)
    {
      service->activate_object();
    }
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error stream;
    stream << FNS
           << "activate_object failure: "
           << exc.what();
    throw Exception(stream);
  }

  logger_->info(
    std::string("Server is succesfully started"),
    Aspect::SERVER);
}

void Server::deactivate_object_()
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
           << exc.what();
    logger_->error(stream.str(), Aspect::SERVER);
  }

  const auto deadline = std::chrono::system_clock::now() +
    std::chrono::milliseconds(100);

  server_->Shutdown(deadline);

  for (auto it = services_.rbegin(); it != services_.rend(); ++it)
  {
    try
    {
      (*it)->deactivate_object();
      (*it)->wait_object();
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
           << exc.what();
    logger_->error(stream.str(), Aspect::SERVER);
  }

  logger_->info(
    std::string("Server is succesfully stopped"),
    Aspect::SERVER);
}

void Server::wait_object_()
{
  UServerUtils::Component::CompositeActiveObjectBase::SimpleActiveObject::wait_object_();
}

void Server::add_channel_args()
{
  namespace Utils = UServerUtils::Grpc::Common::Utils;

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

} // namespace UServerUtils::Grpc::Server
