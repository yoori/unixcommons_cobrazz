// STD
#include <thread>

// THIS
#include <UServerUtils/Grpc/Core/Server/RpcPoolImpl.hpp>
#include <UServerUtils/Grpc/Core/Server/Event.hpp>

namespace UServerUtils::Grpc::Core::Server
{

namespace Aspect
{

const char RPCPOOL[] = "RPCPOOLIMPL";

} // namespace Aspect

RpcPoolImpl::RpcPoolImpl(Logger* logger)
  : logger_(ReferenceCounting::add_ref(logger))
{
  rpcs_.reserve(100000);
}

RpcPoolImpl::~RpcPoolImpl()
{
  using namespace std::chrono_literals;
  try
  {
    Stream::Error stream;
    bool error = false;

    if (state_ == AS_ACTIVE)
    {
      stream << FNS
             << ": wasn't deactivated.";
      error = true;

      std::unique_lock lock(mutex_);
      for (auto& rpc : rpcs_)
      {
        rpc.first->stop();
      }
      lock.unlock();

      bool is_stopped = false;
      while (!is_stopped)
      {
        std::this_thread::sleep_for(200ms);
        lock.lock();
        for (auto& rpc : rpcs_)
        {
          if (!rpc.first->is_stopped())
          {
            is_stopped = false;
            break;
          }
        }
        lock.unlock();
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
      logger_->error(stream.str(), Aspect::RPCPOOL);
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

void RpcPoolImpl::add(const RpcPtr& rpc)
{
  std::lock_guard<std::mutex> lock(mutex_);
  rpcs_.emplace(rpc.get(), rpc);
  if (state_ != AS_ACTIVE)
  {
    rpc->stop();
  }
}

void RpcPoolImpl::remove(Rpc* rpc) noexcept
{
  try
  {
    std::lock_guard<std::mutex> lock(mutex_);
    rpcs_.erase(rpc);
  }
  catch (...)
  {
  }
}

void RpcPoolImpl::activate_object()
{
  std::lock_guard lock(mutex_);
  if (state_ != AS_NOT_ACTIVE)
  {
    Stream::Error stream;
    stream << FNS
           << ": already active";
    throw ActiveObject::AlreadyActive(stream);
  }
  state_ = AS_ACTIVE;
}

void RpcPoolImpl::deactivate_object()
{
  {
    std::lock_guard <std::mutex> lock(mutex_);
    state_ = AS_DEACTIVATING;
    for (auto& rpc: rpcs_)
    {
      rpc.first->stop();
    }
  }

  condition_variable_.notify_all();
}

void RpcPoolImpl::wait_object()
{
  using namespace std::chrono_literals;

  std::unique_lock lock(mutex_);
  condition_variable_.wait(lock, [this] () {
    return state_ != AS_ACTIVE;
  });
  lock.unlock();

  bool is_stopped = true;
  while (!is_stopped)
  {
    std::this_thread::sleep_for(200ms);
    is_stopped = true;
    lock.lock();
    for (auto& rpc : rpcs_)
    {
      if (!rpc.first->is_stopped())
      {
        is_stopped = false;
        break;
      }
    }
    lock.unlock();
  }

  lock.lock();
  if (state_ == AS_DEACTIVATING)
    state_ = AS_NOT_ACTIVE;
}

bool RpcPoolImpl::active()
{
  std::lock_guard lock(mutex_);
  return state_ == AS_ACTIVE;
}

} // namespace UServerUtils::Grpc::Core::Server
