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
    std::lock_guard<std::mutex> lock(mutex_);
    rpcs_.erase(rpc);
  }

  void RpcPoolImpl::deactivate_object_()
  {
    Rpcs rpcs;

    {
      std::lock_guard<std::mutex> lock(mutex_);
      rpcs = rpcs_;
    }

    for (auto& rpc: rpcs)
    {
      rpc.first->stop();
    }
  }

  void RpcPoolImpl::wait_object_()
  {
    Rpcs rpcs;

    {
      std::lock_guard<std::mutex> lock(mutex_);
      rpcs = rpcs_;
    }

    while (true)
    {
      bool is_stopped = true;

      for (auto& rpc: rpcs)
      {
        if (!rpc.first->is_stopped())
        {
          is_stopped = false;
          break;
        }
      }

      if (is_stopped)
      {
        break;
      }

      using namespace std::chrono_literals;
      std::this_thread::sleep_for(200ms);
    }
  }

} // namespace UServerUtils::Grpc::Core::Server
