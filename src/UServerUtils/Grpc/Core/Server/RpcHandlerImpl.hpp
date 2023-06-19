#ifndef GRPC_CORE_SERVER_RPC_HANDLER_IMPL_H_
#define GRPC_CORE_SERVER_RPC_HANDLER_IMPL_H_

// STD
#include <atomic>
#include <memory>

// THIS
#include <eh/Exception.hpp>
#include <Generics/Uncopyable.hpp>
#include <UServerUtils/Grpc/Core/Common/RpcServiceMethodTraits.hpp>
#include <UServerUtils/Grpc/Core/Server/RpcHandler.hpp>
#include <UServerUtils/Grpc/Core/Server/Rpc.hpp>

namespace UServerUtils::Grpc::Core::Server
{

namespace Common = UServerUtils::Grpc::Core::Common;

enum class WriterStatus
{
  Ok = 0,
  RpcClosed,
  InternalError
};

template<class Response>
class Writer : protected Generics::Uncopyable
{
public:
  using ResponsePtr = std::unique_ptr<Response>;

public:
  virtual ~Writer() = default;

  virtual WriterStatus write(ResponsePtr&& response) const noexcept = 0;

  virtual WriterStatus writes_done() const noexcept = 0;

  virtual WriterStatus finish(grpc::Status&& status) const noexcept = 0;

protected:
  Writer() = default;
};

template<typename RpcServiceMethodConcept>
class RpcHandlerImpl : public RpcHandler
{
public:
  using Traits = Common::RpcServiceMethodTraits<RpcServiceMethodConcept>;
  using Request = typename Traits::Request;
  using Response = typename Traits::Response;
  using ResponsePtr = std::unique_ptr<Response>;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

private:
  class WriterImpl final : public Writer<Response>
  {
  public:
    using ResponsePtr = typename Writer<Response>::ResponsePtr;
    using RpcWeakPtr = std::weak_ptr<Rpc>;

  private:
    using Counter = std::atomic<int>;

  public:
    explicit WriterImpl(
      RpcWeakPtr&& rpc,
      const bool need_finish_on_idle = false)
      : rpc_(std::move(rpc)),
        need_finish_on_idle_(need_finish_on_idle)
    {
    }

    ~WriterImpl() override
    {
      if (need_finish_on_idle_)
      {
        const int count = counter_.fetch_sub(
          1,
          std::memory_order_relaxed);
        if (count == 0)
        {
          grpc::Status status(grpc::Status::CANCELLED);
          finish(std::move(status));
        }
      }
    }

    WriterStatus write(ResponsePtr&& response) const noexcept override
    {
      if (auto rpc = rpc_.lock())
      {
        if (rpc->write(std::move(response)))
        {
          if (need_finish_on_idle_)
          {
            counter_.fetch_add(1, std::memory_order_relaxed);
          }

          return WriterStatus::Ok;
        }
        else
        {
          return WriterStatus::InternalError;
        }
      }
      else
      {
        return WriterStatus::RpcClosed;
      }
    }

    WriterStatus writes_done() const noexcept override
    {
      if (auto rpc = rpc_.lock())
      {
        if (rpc->finish(grpc::Status(grpc::Status::OK)))
        {
          if (need_finish_on_idle_)
          {
            counter_.fetch_add(1, std::memory_order_relaxed);
          }

          return WriterStatus::Ok;
        }
        else
        {
          return WriterStatus::InternalError;
        }
      }
      else
      {
        return WriterStatus::RpcClosed;
      }
    }

    WriterStatus finish(grpc::Status&& status) const noexcept override
    {
      if (auto rpc = rpc_.lock())
      {
        if (rpc->finish(std::move(status)))
        {
          if (need_finish_on_idle_)
          {
            counter_.fetch_add(1, std::memory_order_relaxed);
          }

          return WriterStatus::Ok;
        }
        else
        {
          return WriterStatus::InternalError;
        }
      }
      else
      {
        return WriterStatus::RpcClosed;
      }
    }

  private:
    friend class RpcHandlerImpl;

    const RpcWeakPtr rpc_;

    const bool need_finish_on_idle_ = false;

    mutable Counter counter_{0};
  };

public:
  RpcHandlerImpl() = default;

  ~RpcHandlerImpl() override = default;

  bool send(ResponsePtr&& response) noexcept
  {
    try
    {
      using Message = google::protobuf::Message;
      std::unique_ptr<Message> message(response.release());
      return rpc_->write(std::move(message));
    }
    catch (...)
    {
      return false;
    }
  }

  bool finish(grpc::Status&& status) noexcept
  {
    return rpc_->finish(std::move(status));
  }

  std::unique_ptr<Writer<Response>> get_writer(
    const bool need_finish_on_idle = false)
  {
    return std::make_unique<WriterImpl>(
      rpc_->get_weak_ptr(),
      need_finish_on_idle);
  }

  virtual void on_request(const Request& request) = 0;

  template<class T>
  T& common_context()
  {
    if (!common_context_)
    {
      Stream::Error stream;
      stream << FNS
             << ": common_context is null";
      throw Exception(stream);
    }

    return static_cast<T&>(*common_context_);
  }

private:
  void set_rpc(Rpc* rpc) noexcept override
  {
    rpc_ = rpc;
  }

  void set_common_context(
    CommonContext* common_context) noexcept override
 {
   common_context_ = CommonContext_var(
     ReferenceCounting::add_ref(common_context));
 }

  void on_request_internal(
    const google::protobuf::Message& request) override final
  {
    static_assert(std::is_base_of_v<google::protobuf::Message, Request>,
                  "Request must be derived from google::protobuf::Message");
    on_request(static_cast<const Request&>(request));
  }

private:
  Rpc* rpc_ = nullptr;

  CommonContext_var common_context_;
};

} // namespace UServerUtils::Grpc::Core::Server

#endif //GRPC_CORE_SERVER_RPC_HANDLER_IMPL_H_
