#ifndef GRPC_CORE_COMMON_SCHEDULER_H_
#define GRPC_CORE_COMMON_SCHEDULER_H_

// GRPC
#include <grpc++/grpc++.h>

// STD
#include <condition_variable>
#include <memory>
#include <vector>

// THIS
#include <Generics/ActiveObject.hpp>
#include <Logger/Logger.hpp>
#include "ThreadGuard.hpp"

namespace UServerUtils::Grpc::Core::Common
{

class Scheduler final
  : public Generics::ActiveObject,
    public ReferenceCounting::AtomicImpl
{
public:
  using Logger_var = Logging::Logger_var;
  using Queue = std::shared_ptr<grpc::CompletionQueue>;
  using Queues = std::vector<Queue>;

public:
  explicit Scheduler(
    const Logger_var& logger,
    Queues&& queues);

  ~Scheduler() override;

  void activate_object() override;

  void deactivate_object() override;

  void wait_object() override;

  bool active() override;

  std::size_t size() const noexcept;

private:
  Logger_var logger_;

  Queues queues_;

  ThreadsGuard threads_;

  ACTIVE_STATE state_ = AS_NOT_ACTIVE;

  mutable std::mutex mutex_;

  std::condition_variable condition_variable_;
};

using Scheduler_var = ReferenceCounting::SmartPtr<Scheduler>;

} // namespace UServerUtils::Grpc::Core::Common

#endif // GRPC_CORE_COMMON_SCHEDULER_H_
