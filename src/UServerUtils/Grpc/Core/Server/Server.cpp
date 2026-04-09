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
      const auto best_thread_number = std::thread::hardware_concurrency();
      if (best_thread_number == 0)
      {
        Stream::Error stream;
        stream << FNS << ": hardware_concurrency is failed";
        logger_->error(stream.str(), Aspect::SERVER);
      }
      config_.num_threads = best_thread_number ? best_thread_number : 128;
    }

    Common::Scheduler::Queues queues;
    server_completion_queues_.reserve(*config_.num_threads);
    for (std::size_t i = 0; i < *config_.num_threads; ++i)
    {
      std::shared_ptr<grpc::ServerCompletionQueue>
        server_completion_queue(
          server_builder_.AddCompletionQueue());
      server_completion_queues_.emplace_back(
        server_completion_queue);
      queues.emplace_back(server_completion_queue);
    }

    scheduler_ = Common::SchedulerPtr(new Common::Scheduler(
      logger_.in(),
      std::move(queues)));

    add_child_object(rpc_pool_);

    if (config_.common_context)
    {
      add_child_object(config_.common_context);
    }
  }

  Server::~Server()
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
      std::cerr << FNS << ": eh::Exception: " << exc.what() << std::endl;
    }
  }

  void Server::activate_object()
  {
    std::ostringstream stream;
    stream << FNS << ": Starting server on ip=" << config_.ip <<
      " , port=" << config_.port << "...";
    logger_->info(stream.str(), Aspect::SERVER);

    try
    {
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
    }
    catch (const eh::Exception& exc)
    {
      Stream::Error stream;
      stream << FNS << ": activate_object failure: " << exc.what();
      throw Exception(stream);
    }

    Generics::CompositeActiveObject::activate_object();

    logger_->info(
      std::string("Server is succesfully started"),
      Aspect::SERVER);
  }

  void Server::deactivate_object()
  {
    server_->Shutdown(
      std::chrono::system_clock::now() +
      std::chrono::milliseconds(100));

    Generics::CompositeActiveObject::deactivate_object();

    logger_->info(
      std::string("Server is succesfully stopped"),
      Aspect::SERVER);
  }

  void Server::add_channel_args()
  {
    for (const auto& [name, value] : config_.channel_args)
    {
      if (UServerUtils::Grpc::Core::Common::Utils::is_integer(value))
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
        handlers.emplace_back(method_name, rpc_handler_info);
      }

      Service_var service(new Service(
        logger_.in(),
        rpc_pool_.in(),
        server_completion_queues_,
        config_.common_context,
        std::move(handlers)));
      add_child_object(service);
      server_builder_.RegisterService(service.in());
      services_.emplace_back(std::move(service));
    }
  }
} // namespace UServerUtils::Grpc::Core::Server
