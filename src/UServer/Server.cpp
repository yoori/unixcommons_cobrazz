#include <iostream>

#include <UServer/Server.hpp>

#include <userver/logging/component.hpp>
#include <userver/os_signals/component.hpp>
#include <userver/components/tracer.hpp>
#include <userver/components/statistics_storage.hpp>

#include <userver/components/run.hpp>

namespace UServer
{
  //
  // class Server
  //

  Server::Server(unsigned short grpc_port)
    /*throw (eh::Exception)*/
  {
    components_config_ += "        grpc-server:\n";
    components_config_ += "            port: ";
    components_config_ += std::to_string(grpc_port);
    components_config_ += "\n";
    (*component_list_)
      .Append<userver::os_signals::ProcessorComponent>()
      .Append<userver::components::Logging>()
      .Append<userver::components::Tracer>() // used in StatisticsStorage
      .Append<userver::components::StatisticsStorage>();
  }

  Server::~Server() throw ()
  {
    if (state_ != AS_NOT_ACTIVE)
    {
      std::cerr << "UServer::Server is not deactivated" << std::endl;
    }
  }

  void
  Server::activate_object()
    /*throw (AlreadyActive, Exception, eh::Exception)*/
  {
    state_ = AS_ACTIVE;

    std::string config_str = R"(
# yaml
components_manager:
    components:
        # The required common components
        logging:
            fs-task-processor: fs-task-processor
            loggers:
                default:
                    file_path: '@stderr'
                    level: warning
                    overflow_behavior: discard
        tracer:
            service-name: grpc-service'

)";
    config_str += components_config_;
    config_str += R"(
    default_task_processor: main-task-processor
# /// [gRPC sample - task processor]
# yaml
    task_processors:
        grpc-blocking-task-processor:  # For blocking gRPC channel creation
            thread_name: grpc-worker
            worker_threads: 2
# /// [gRPC sample - task processor]
        main-task-processor:           # For non-blocking operations
            thread_name: main-worker
            worker_threads: 4
        fs-task-processor:             # For blocking filesystem operations
            thread_name: fs-worker
            worker_threads: 2
    coro_pool:
        initial_size: 500
        max_size: 10000
)";
    thread_ = std::thread([component_list = component_list_, config_str]() {
        userver::components::InMemoryConfig config(config_str);
        userver::components::Run(
            config,
            *component_list,
            {}, // init_log_path
            userver::logging::Format::kTskv);
    });
  }

  void
  Server::deactivate_object()
    /*throw (Exception, eh::Exception)*/
  {
    state_ = AS_DEACTIVATING;
  }

  void
  Server::wait_object()
    /*throw (Exception, eh::Exception)*/
  {
    state_ = AS_NOT_ACTIVE;
  }

  bool
  Server::active()
    /*throw (eh::Exception)*/
  {
    return state_ == AS_ACTIVE;
  }
}

