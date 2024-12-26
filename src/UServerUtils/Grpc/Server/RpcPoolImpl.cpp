// STD
#include <thread>

// THIS
#include <UServerUtils/Grpc/Server/RpcPoolImpl.hpp>
#include <UServerUtils/Grpc/Server/Event.hpp>

namespace UServerUtils::Grpc::Server
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
  try
  {
    deactivate_object();
    wait_object();
  }
  catch (...)
  {
  }
}

void RpcPoolImpl::add(const RpcPtr& rpc)
{
  std::lock_guard<std::mutex> lock(mutex_);
  rpcs_.emplace(rpc.get(), rpc);
  if (!active())
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

void RpcPoolImpl::deactivate_object_()
{
  std::lock_guard <std::mutex> lock(mutex_);
  for (auto& rpc: rpcs_)
  {
    rpc.first->stop();
  }
}

void RpcPoolImpl::wait_object_()
{
  bool is_stopped = false;
  std::unique_lock lock(mutex_, std::defer_lock);
  while (!is_stopped)
  {
    std::this_thread::sleep_for(
      std::chrono::milliseconds(200));
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
}

} // namespace UServerUtils::Grpc::Server
