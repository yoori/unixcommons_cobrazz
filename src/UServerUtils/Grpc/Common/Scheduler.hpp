#ifndef GRPC_COMMON_SCHEDULER_H_
#define GRPC_COMMON_SCHEDULER_H_

// GRPC
#include <grpc++/grpc++.h>

// STD
#include <condition_variable>
#include <memory>
#include <vector>

// THIS
#include <eh/Exception.hpp>
#include <Logger/Logger.hpp>
#include <UServerUtils/Grpc/Common/ThreadGuard.hpp>

namespace UServerUtils::Grpc::Common
{

class Scheduler final
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using Queue = std::shared_ptr<grpc::CompletionQueue>;
  using Queues = std::vector<Queue>;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  explicit Scheduler(
    Logger* logger,
    Queues&& queues);

  ~Scheduler();

  std::size_t size() const noexcept;

  const Queues& queues() noexcept;

private:
  Logger_var logger_;

  Queues queues_;

  ThreadsGuard threads_;
};

using SchedulerPtr = std::shared_ptr<Scheduler>;

} // namespace UServerUtils::Grpc::Common

#endif // GRPC_COMMON_SCHEDULER_H_
