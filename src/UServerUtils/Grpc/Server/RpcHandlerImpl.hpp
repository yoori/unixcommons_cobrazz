#ifndef GRPC_SERVER_RPC_HANDLER_IMPL_H_
#define GRPC_SERVER_RPC_HANDLER_IMPL_H_

// STD
#include <atomic>
#include <memory>

// THIS
#include <eh/Exception.hpp>
#include <Generics/Uncopyable.hpp>
#include <UServerUtils/Grpc/Common/RpcServiceMethodTraits.hpp>
#include <UServerUtils/Grpc/Server/DefaultErrorCreator.hpp>
#include <UServerUtils/Grpc/Server/RpcHandler.hpp>
#include <UServerUtils/Grpc/Server/Rpc.hpp>

namespace UServerUtils::Grpc::Server
{

namespace Common = UServerUtils::Grpc::Common;

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
  using RequestPtr = std::unique_ptr<Request>;
  using Response = typename Traits::Response;
  using ResponsePtr = std::unique_ptr<Response>;
  using Message = typename RpcHandler::Message;
  using MessagePtr = typename RpcHandler::MessagePtr;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

private:
  class WriterImpl final : public Writer<Response>
  {
  public:
    using ResponsePtr = typename Writer<Response>::ResponsePtr;
    using RpcWeakPtr = std::weak_ptr<Rpc>;

  private:
    using RpcType = grpc::internal::RpcMethod::RpcType;
    using Counter = std::atomic<int>;

  public:
    explicit WriterImpl(
      RpcWeakPtr&& rpc,
      const RpcType rpc_type,
      DefaultErrorCreatorPtr<Response>&& default_error_creator,
      const std::optional<grpc::StatusCode>& finish_status_code = {})
      : rpc_(std::move(rpc)),
        rpc_type_(rpc_type),
        default_error_creator_(std::move(default_error_creator)),
        finish_status_code_(std::move(finish_status_code))
    {
    }

    ~WriterImpl() override
    {
      const int count = counter_.fetch_sub(
        1,
        std::memory_order_relaxed);
      if (count == 0)
      {
        if (default_error_creator_)
        {
          auto error = default_error_creator_->create();
          write(std::move(error));
        }
        else if (finish_status_code_)
        {
          grpc::Status status(
            *finish_status_code_,
            std::string{});
          finish(std::move(status));
        }
      }

      if (rpc_type_ == RpcType::SERVER_STREAMING)
      {
        const int count = finish_counter_.fetch_sub(
          1,
          std::memory_order_relaxed);
        if (count == 0)
        {
          grpc::Status status = grpc::Status::CANCELLED;
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
          counter_.fetch_add(1, std::memory_order_relaxed);
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
          counter_.fetch_add(1, std::memory_order_relaxed);
          if (rpc_type_ == RpcType::SERVER_STREAMING)
          {
            finish_counter_.fetch_add(1, std::memory_order_relaxed);
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
          counter_.fetch_add(1, std::memory_order_relaxed);
          if (rpc_type_ == RpcType::SERVER_STREAMING)
          {
            finish_counter_.fetch_add(1, std::memory_order_relaxed);
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

    const RpcType rpc_type_;

    const DefaultErrorCreatorPtr<Response> default_error_creator_;

    const std::optional<grpc::StatusCode> finish_status_code_;

    mutable Counter counter_{0};

    mutable Counter finish_counter_{0};
  };

public:
  RpcHandlerImpl() = default;

  ~RpcHandlerImpl() override = default;

  bool send(ResponsePtr&& response) noexcept
  {
    using Message = google::protobuf::Message;
    std::unique_ptr<Message> message(response.release());
    return rpc_->write(std::move(message));
  }

  bool finish(grpc::Status&& status) noexcept
  {
    return rpc_->finish(std::move(status));
  }

  /**
   * If there was no recording, then we won’t give anything
   */
  std::unique_ptr<Writer<Response>> get_writer()
  {
    return std::make_unique<WriterImpl>(
      rpc_->get_weak_ptr(),
      Traits::rpc_type,
      DefaultErrorCreatorPtr<Response>{},
      std::nullopt);
  }

  /**
   * If there was no record, then we send an error by default
   */
  std::unique_ptr<Writer<Response>> get_writer(const Request& request)
  {
    return std::make_unique<WriterImpl>(
      rpc_->get_weak_ptr(),
      Traits::rpc_type,
      default_error_creator(request),
      std::nullopt);
  }

  /**
   * If there was no recording, then we send finish with the status
   */
  std::unique_ptr<Writer<Response>> get_writer(const grpc::StatusCode& status_code)
  {
    return std::make_unique<WriterImpl>(
      rpc_->get_weak_ptr(),
      Traits::rpc_type,
      DefaultErrorCreatorPtr<Response>{},
      status_code);
  }

  virtual DefaultErrorCreatorPtr<Response>
  default_error_creator(const Request& /*request*/) noexcept
  {
    return {};
  }

  virtual void on_request(const Request& /*request*/)
  {
  }

  virtual void on_request(RequestPtr&& /*request*/)
  {
  }

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

  void set_common_context(CommonContext* common_context) noexcept override
 {
   common_context_ = CommonContext_var(
     ReferenceCounting::add_ref(common_context));
 }

  void on_request_internal(const Message& request) override final
  {
    static_assert(std::is_base_of_v<Message, Request>,
                  "Request must be derived from google::protobuf::Message");
    on_request(static_cast<const Request&>(request));
  }

  void on_request_internal(MessagePtr&& request) override final
  {
    static_assert(std::is_base_of_v<Message, Request>,
                  "Request must be derived from google::protobuf::Message");
    on_request(std::unique_ptr<Request>(static_cast<Request*>(request.release())));
  }

private:
  Rpc* rpc_ = nullptr;

  CommonContext_var common_context_;
};

} // namespace UServerUtils::Grpc::Server

#endif //GRPC_SERVER_RPC_HANDLER_IMPL_H_