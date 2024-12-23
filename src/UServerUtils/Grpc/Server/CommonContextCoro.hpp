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
#include <UServerUtils/Grpc/Server/ServiceCoro.hpp>

namespace UServerUtils::Grpc::Server
{

namespace Aspect
{

extern const char* COMMON_CONTEXT_CORO;

} // namespace Aspect

class CommonContextCoro final :
  public CommonContext,
  public ReferenceCounting::AtomicImpl
{
public:
  using MaxSizeQueue = std::optional<std::size_t>;
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using TaskProcessor = userver::engine::TaskProcessor;
  using TaskProcessorRef = std::reference_wrapper<TaskProcessor>;
  using MethodName = std::string_view;

private:
  struct ServiceInfo final
  {
    ServiceInfo(
      const std::any& service,
      const grpc::internal::RpcMethod::RpcType rpc_type,
      TaskProcessor& task_processor)
      : service(service),
        rpc_type(rpc_type),
        task_processor(task_processor)
    {
    }

    ~ServiceInfo() = default;

    ServiceInfo(const ServiceInfo&) = default;
    ServiceInfo(ServiceInfo&&) = default;
    ServiceInfo& operator=(const ServiceInfo&) = default;
    ServiceInfo& operator=(ServiceInfo&&) = default;

    std::any service;
    grpc::internal::RpcMethod::RpcType rpc_type;
    TaskProcessorRef task_processor;
  };

  using Services = std::unordered_map<MethodName, ServiceInfo>;
  using Queues = std::unordered_map<MethodName, std::any>;
  using Producers = std::deque<std::any>;
  using TaskWithResult = userver::engine::TaskWithResult<void>;
  using WorkerTasks = std::deque<TaskWithResult>;

  template<class Request, class Response>
  using Reader = Internal::Reader<Request, Response>;
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

  template<class Request, class Response>
  auto get_producer(const MethodName method_name)
  {
    using Queue = Internal::QueueCoro<Request, Response>;
    using QueuePtr = std::shared_ptr<Queue>;
    using Producer = typename Queue::Producer;
    using Reader = Reader<Request, Response>;
    using ReaderPtr = ReaderPtr<Request, Response>;

    auto it_queue = queues_.find(method_name);
    if (it_queue != queues_.end())
    {
      const auto queue = std::any_cast<QueuePtr>(it_queue->second);
      return std::make_unique<Producer>(queue->GetProducer());
    }

    const auto queue = Queue::Create(
      max_size_queue_ ? *max_size_queue_ : Queue::kUnbounded);

    auto it_service = services_.find(method_name);
    if (it_service == services_.end())
    {
      Stream::Error stream;
      stream << FNS
             << "not existing service for name="
             << method_name;
      throw Exception(stream);
    }
    const auto& service_info = it_service->second;
    const auto service =
      std::any_cast<ServiceCoro_var<Request, Response>>(
        service_info.service);
    auto& task_processor = service_info.task_processor;

    auto producer = std::make_unique<Producer>(
      queue->GetProducer());

    ReaderPtr reader(new Reader(
      logger_.in(),
      queue->GetConsumer()));

    add_coroutine(
      task_processor,
      std::move(reader),
      service.in(),
      true,
      service_info.rpc_type).Detach();

    return producer;
  }

  template<class Request, class Response>
  void add_service(
    const MethodName& method_name,
    ServiceCoro<Request, Response>* service,
    const grpc::internal::RpcMethod::RpcType rpc_type,
    TaskProcessor& task_processor,
    const std::optional<std::size_t> number_coro)
  {
    using Queue = Internal::QueueCoro<Request, Response>;
    using QueuePtr = std::shared_ptr<Queue>;
    using Producer = typename Queue::Producer;
    using Reader = Reader<Request, Response>;
    using ReaderPtr = ReaderPtr<Request, Response>;

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
             << ": service whith method_name="
             << method_name
             << " already exist";
      throw Exception(stream);
    }

    ServiceInfo service_info(
      ServiceCoro_var<Request, Response>(ReferenceCounting::add_ref(service)),
      rpc_type,
      task_processor);
    services_.try_emplace(method_name, service_info);

    if (number_coro)
    {
      QueuePtr queue = Queue::Create(
        max_size_queue_ ? *max_size_queue_ : Queue::kUnbounded);
      if (!queue)
      {
        Stream::Error stream;
        stream << FNS
               << ": Queue::Create return null ptr";
        throw Exception(stream);
      }

      producers_.emplace_back(
        std::make_shared<Producer>(
          queue->GetProducer()));
      queues_[method_name] = queue;

      for (std::size_t i = 1; i <= *number_coro; ++i)
      {
        worker_tasks_.emplace_back(
          add_coroutine(
            task_processor,
            ReaderPtr(new Reader(
              logger_,
              queue->GetConsumer())),
            service,
            false,
            rpc_type));
      }
    }
  }

protected:
  ~CommonContextCoro() override;

private:
  void activate_object_() override;

  void deactivate_object_() override;

  template<class Request, class Response>
  auto add_coroutine(
    TaskProcessor& task_processor,
    ReaderPtr<Request, Response>&& reader,
    ServiceCoro<Request, Response>* service,
    const bool is_coro_per_rpc,
    const grpc::internal::RpcMethod::RpcType rpc_type)
  {
    return userver::engine::AsyncNoSpan(
      task_processor,
      [logger = logger_,
       reader = std::move(reader),
       service = ServiceCoro_var<Request, Response>(
         ReferenceCounting::add_ref(service)),
       is_coro_per_rpc,
       rpc_type] () mutable {
        while (!reader->is_finish())
        {
          try
          {
            service->handle(*reader);
            userver::engine::Yield();
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
              userver::engine::Yield();
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
              userver::engine::Yield();
            }
            catch (...)
            {
            }
          }

          if (is_coro_per_rpc
          && (rpc_type == grpc::internal::RpcMethod::RpcType::NORMAL_RPC
           || rpc_type == grpc::internal::RpcMethod::RpcType::SERVER_STREAMING))
          {
            break;
          }
        }
      }
    );
  }
  
private:
  Logger_var logger_;

  const MaxSizeQueue max_size_queue_;

  Services services_;

  Queues queues_;

  Producers producers_;

  WorkerTasks worker_tasks_;
};

using CommonContextCoro_var = ReferenceCounting::SmartPtr<CommonContextCoro>;

} // namespace UServerUtils::Grpc::Server

#endif // GRPC_SERVER_COMMON_CONTEXT_H_