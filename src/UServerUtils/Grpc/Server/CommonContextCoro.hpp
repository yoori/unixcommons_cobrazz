#ifndef GRPC_SERVER_COMMON_CONTEXT_CORO_H_
#define GRPC_SERVER_COMMON_CONTEXT_CORO_H_

// STD
#include <any>
#include <deque>

// USERVER
#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task_processor.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/concurrent/queue.hpp>

// THIS
#include <Logger/Logger.hpp>
#include <UServerUtils/Grpc/Server/CommonContext.hpp>
#include <UServerUtils/Grpc/Server/DefaultErrorCreator.hpp>
#include <UServerUtils/Grpc/Server/ServiceCoro.hpp>

namespace UServerUtils::Grpc::Server
{

namespace Aspect
{

extern const char* COMMON_CONTEXT_CORO;

} // namespace Aspect

template<class Request, class Response>
class CoroFactoryDefaultErrorCreator final
  : public FactoryDefaultErrorCreator<Request, Response>
{
public:
  CoroFactoryDefaultErrorCreator(
    ServiceCoro<Request, Response>* service_coro)
    : service_coro_(ReferenceCounting::add_ref(service_coro))
  {
  }

  ~CoroFactoryDefaultErrorCreator() override = default;

  DefaultErrorCreatorPtr<Response> create(
    const Request& request) noexcept override
  {
    return service_coro_->default_error_creator(request);
  }

private:
  ServiceCoro_var<Request, Response> service_coro_;
};

class CommonContextCoro final :
  public CommonContext,
  public ReferenceCounting::AtomicImpl
{
public:
  using MaxSizeQueue = std::optional<std::size_t>;
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using TaskProcessor = userver::engine::TaskProcessor;
  using MethodName = std::string_view;
  using IdRpc = Internal::Types::IdRpc;
  using RpcType = grpc::internal::RpcMethod::RpcType;

private:
  struct ServiceInfo final
  {
    explicit ServiceInfo(
      const std::any& service,
      const RpcType rpc_type,
      const ServiceMode service_mode,
      TaskProcessor& task_processor)
      : service(service),
        rpc_type(rpc_type),
        service_mode(service_mode),
        task_processor(&task_processor)
    {
    }

    std::any service;
    const RpcType rpc_type;
    const ServiceMode service_mode;
    TaskProcessor* task_processor;
  };

  using Services = std::unordered_map<MethodName, ServiceInfo>;
  template<class Request, class Response>
  using ReaderPtr = Internal::ReaderPtr<Request, Response>;

public:
  explicit CommonContextCoro(
    Logger* logger,
    const MaxSizeQueue max_size_queue = {});

  Logger_var get_logger() noexcept
  {
    return logger_;
  }

  const ServiceInfo& service_info(const MethodName method_name) const
  {
    auto it_service = services_.find(method_name);
    if (it_service == services_.end())
    {
      Stream::Error stream;
      stream << FNS
             << "not existing service for name="
             << method_name;
      throw Exception(stream);
    }

    return it_service->second;
  }

  template<class Request, class Response>
  FactoryDefaultErrorCreatorPtr<Request, Response> factory_default_error_creator(
    const ServiceInfo& service_info)
  {
    const auto& service = std::any_cast<
      ServiceCoro_var<Request, Response>>(
        service_info.service);

    return std::make_unique<
      CoroFactoryDefaultErrorCreator<Request, Response>>(
        service.in());
  }

  template<class Request, class Response>
  void add_coroutine(
    TaskProcessor& task_processor,
    ServiceCoro<Request, Response>* service,
    const ReadStatus status,
    std::unique_ptr<Writer<Response>>&& writer,
    std::unique_ptr<Request>&& request,
    const IdRpc id_rpc,
    const RpcType rpc_type)
  {
    using ReaderPtr = ReaderPtr<Request, Response>;

    try
    {
      ReaderPtr reader = std::make_unique<
        Internal::SingleReader<Request, Response>>(
        status,
        std::move(writer),
        std::move(request),
        id_rpc);

      const bool is_critical =
        status == ReadStatus::Initialize ||
        status == ReadStatus::ReadsDone ||
        status == ReadStatus::RpcFinish;
      add_coroutine(
        task_processor,
        std::move(reader),
        service,
        rpc_type,
        is_critical).Detach();
    }
    catch (const eh::Exception& exc)
    {
      Stream::Error stream;
      stream << FNS
             << exc.what();
      throw Exception(stream);
    }
    catch (...)
    {
      Stream::Error stream;
      stream << FNS
             << "Unknown error";
      throw Exception(stream);
    }
  }

  template<class Request, class Response>
  auto get_producer(const ServiceInfo& service_info)
  {
    using Queue = Internal::QueueCoro<Request, Response>;
    using Producer = typename Queue::Producer;
    using ReaderPtr = ReaderPtr<Request, Response>;

    const auto queue = Queue::Create(
      max_size_queue_ ? *max_size_queue_ : Queue::kUnbounded);

    const auto& service = std::any_cast<
      ServiceCoro_var<Request, Response>>(
        service_info.service);
    auto& task_processor = *service_info.task_processor;

    auto producer = std::make_unique<Producer>(
      queue->GetProducer());

    ReaderPtr reader = std::make_unique<
      Internal::QueueReader<Request, Response>>(
        logger_.in(),
        queue->GetConsumer());

    add_coroutine(
      task_processor,
      std::move(reader),
      service.in(),
      service_info.rpc_type,
      true).Detach();

    return producer;
  }

  template<class Request, class Response>
  void add_service(
    const MethodName& method_name,
    ServiceCoro<Request, Response>* service,
    const RpcType rpc_type,
    const ServiceMode service_mode,
    TaskProcessor& task_processor)
  {
    if (active())
    {
      Stream::Error stream;
      stream << FNS
             << "Can't add service: CommonContextCoro already active";
      throw Exception(stream);
    }

    if (services_.count(method_name) != 0)
    {
      Stream::Error stream;
      stream << FNS
             << "Service whith method_name="
             << method_name
             << " already exist";
      throw Exception(stream);
    }

    ServiceInfo service_info(
      ServiceCoro_var<Request, Response>(
        ReferenceCounting::add_ref(service)),
      rpc_type,
      service_mode,
      task_processor);
    services_.try_emplace(method_name, service_info);
  }

protected:
  ~CommonContextCoro() override;

private:
  template<class Request, class Response>
  auto add_coroutine(
    TaskProcessor& task_processor,
    ReaderPtr<Request, Response>&& reader,
    ServiceCoro<Request, Response>* service,
    const RpcType rpc_type,
    const bool is_critical)
  {
    auto function = [logger = logger_,
      reader = std::move(reader),
      service = ServiceCoro_var<Request, Response>(
        ReferenceCounting::add_ref(service)),
      rpc_type] () mutable {
        while (!reader->is_finish())
        {
          try
          {
            service->handle(*reader);
          }
          catch (const eh::Exception& exc)
          {
            try
            {
              Stream::Error stream;
              stream << FNS
                     << exc.what();
              logger->error(
                stream.str(),
                Aspect::COMMON_CONTEXT_CORO);
            }
            catch (...)
            {
            }
          }
          catch (...)
          {
            try
            {
              Stream::Error stream;
              stream << FNS
                     << "Unknown error";
              logger->error(
                stream.str(),
                Aspect::COMMON_CONTEXT_CORO);
            }
            catch (...)
            {
            }
          }

          if (rpc_type == RpcType::NORMAL_RPC
            || rpc_type == RpcType::SERVER_STREAMING)
          {
            break;
          }

          if (!reader->is_finish())
          {
            userver::engine::Yield();
          }
        }
      };

    if (is_critical)
    {
      return userver::engine::CriticalAsyncNoSpan(
        task_processor,
        std::move(function));
    }
    else
    {
      return userver::engine::AsyncNoSpan(
        task_processor,
        std::move(function));
    }
  }
  
private:
  Logger_var logger_;

  const MaxSizeQueue max_size_queue_;

  Services services_;
};

using CommonContextCoro_var = ReferenceCounting::SmartPtr<CommonContextCoro>;

} // namespace UServerUtils::Grpc::Server

#endif // GRPC_SERVER_COMMON_CONTEXT_H_