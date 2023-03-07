#ifndef USERVER_GRPC_CONFIG_HPP
#define USERVER_GRPC_CONFIG_HPP

// STD
#include <string>

// GRPCPP
#include <grpcpp/security/credentials.h>

namespace UServerUtils::Grpc
{

// Сoroutine pool parameters
struct CoroPoolConfig final
{
  CoroPoolConfig() = default;
  ~CoroPoolConfig() = default;

  std::size_t initial_size = 1000;
  std::size_t max_size = 10000;
  std::size_t stack_size = 256 * 1024ULL;
};

// Parameters of pool event thread(libevent)
struct EventThreadPoolConfig final
{
  EventThreadPoolConfig() = default;
  ~EventThreadPoolConfig() = default;

  std::size_t threads = 2;
  std::string thread_name = "event-worker";
  /* the default loop is the only one that handles signals and child watchers */
  bool ev_default_loop_disabled = true;
  bool defer_events = false;
};

// Parameters of worker pool thread
struct TaskProcessorConfig final
{
  enum class OsScheduling
  {
    Normal,
    LowPriority,
    Idle,
  };

  enum class OverloadAction
  {
    Cancel,   // cancells the tasks
    Ignore    // ignore the tasks
  };

  TaskProcessorConfig() = default;
  ~TaskProcessorConfig() = default;

  std::string name;
  bool should_guess_cpu_limit = false;
  std::size_t worker_threads = 10;
  std::string thread_name;
  OsScheduling os_scheduling = OsScheduling::Normal;

  // Action to perform on taks on queue overload.
  OverloadAction overload_action = OverloadAction::Ignore;
  // Queue size after which the `action` is applied.
  std::size_t wait_queue_length_limit = 100000;
  // Wait in queue time after which the `action is applied`
  std::chrono::microseconds wait_queue_time_limit{0};
  // Wait in queue time after which the overload events for
  // RPS congestion control are generated.
  std::chrono::microseconds sensor_wait_queue_time_limit{10000};
};

struct GrpcServerConfig final
{
  GrpcServerConfig() = default;
  ~GrpcServerConfig() = default;

  // The port to listen to. If `0`, a free port will be picked automatically.
  // If none, the ports have to be configured programmatically using
  // Server::WithServerBuilder.
  std::optional<int> port{};

  // Optional grpc-core channel args
  // @see https://grpc.github.io/grpc/core/group__grpc__arg__keys.html
  std::unordered_map<std::string, std::string> channel_args;

  // Serve a web page with runtime info about gRPC connections
  bool enable_channelz = false;

  bool cancel_task_by_deadline = true;
};

struct GrpcClientFactoryConfig final {
  // gRPC channel credentials, none by default
  std::shared_ptr<grpc::ChannelCredentials> credentials{
    grpc::InsecureChannelCredentials()};

  // Optional grpc-core channel args
  // @see https://grpc.github.io/grpc/core/group__grpc__arg__keys.html
  grpc::ChannelArguments channel_args;

  // Number of underlying channels that will be created for every client
  // in this factory.
  std::size_t channel_count = 1;

  bool enable_deadline_propagation = true;
};

} // namespace UServerUtils::Grpc

#endif //USERVER_GRPC_CONFIG_HPP
