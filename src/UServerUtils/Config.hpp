#ifndef USERVER_CONFIG_HPP
#define USERVER_CONFIG_HPP

// STD
#include <string>

// GRPCPP
#include <grpcpp/security/credentials.h>

namespace UServerUtils
{

// Сoroutine pool parameters
struct CoroPoolConfig final
{
  std::size_t initial_size = 1000;
  std::size_t max_size = 10000;
  std::size_t stack_size = 256 * 1024ULL;
};

// Parameters of pool event thread(libevent)
struct EventThreadPoolConfig final
{
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

} // namespace UServerUtils

#endif //USERVER_CONFIG_HPP
