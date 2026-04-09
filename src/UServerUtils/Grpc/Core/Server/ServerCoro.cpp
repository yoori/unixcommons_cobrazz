// THIS
#include <UServerUtils/Grpc/Core/Server/ServerCoro.hpp>

namespace UServerUtils::Grpc::Core::Server
{
  namespace Aspect
  {
    const char SERVER_CORO[] = "SERVER_CORO";
  } // namespace Aspect

  ServerCoro::ServerCoro(
    const ConfigCoro& config,
    Logging::Logger* logger)
    : logger_(ReferenceCounting::add_ref(logger)),
      common_context_coro_(new CommonContextCoro(logger, config.max_size_queue))
  {
    Config config_server;
    config_server.ip = config.ip;
    config_server.port = config.port;
    config_server.num_threads = config.num_threads;
    config_server.channel_args = config.channel_args;
    config_server.common_context = common_context_coro_;

    server_ = ReferenceCounting::SmartPtr<Server>(new Server(
      config_server,
      logger_));
    add_child_object(server_);
  }

  ServerCoro::~ServerCoro()
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
} // namespace UServerUtils::Grpc::Core::Server
