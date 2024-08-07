#ifndef GRPC_COMMON_SHUTDAUN_MANAGER_H_
#define GRPC_COMMON_SHUTDAUN_MANAGER_H_

// STD
#include <condition_variable>
#include <mutex>

// THIS
#include <Generics/Uncopyable.hpp>

namespace UServerUtils::Grpc::Common
{

class ShutdownManager final
  : protected Generics::Uncopyable
{
public:
  ShutdownManager() = default;

  ~ShutdownManager() = default;

  void shutdown()
  {
    std::unique_lock lock(mutex_);
    is_shutdown_ = true;
    cv_.notify_all();
  }

  void wait()
  {
    std::unique_lock <std::mutex> lock(mutex_);
    cv_.wait(lock, [this] {
      return is_shutdown_;
    });
  }

private:
  bool is_shutdown_ = false;

  std::condition_variable cv_;

  mutable std::mutex mutex_;
};

using ShutdownManagerPtr = std::shared_ptr<ShutdownManager>;

} // namespace UServerUtils::Grpc::Common

#endif // GRPC_COMMON_SHUTDAUN_MANAGER_H_
