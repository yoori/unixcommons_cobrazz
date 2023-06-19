#ifndef GRPC_CORE_SERVER_SERVER_H_
#define GRPC_CORE_SERVER_SERVER_H_

// GRPC
#include <grpcpp/grpcpp.h>

// PROTOBUF
#include <google/protobuf/message.h>

// THIS
#include <Generics/CompositeActiveObject.hpp>
#include <Logger/Logger.hpp>
#include <UServerUtils/Grpc/Core/Server/Config.hpp>
#include <UServerUtils/Grpc/Core/Common/Utils.hpp>
#include <UServerUtils/Grpc/Core/Server/RpcHandlerInfo.hpp>
#include <UServerUtils/Grpc/Core/Server/RpcPoolImpl.hpp>
#include <UServerUtils/Grpc/Core/Server/Service.hpp>
#include <UServerUtils/Grpc/Core/Common/Scheduler.hpp>

namespace UServerUtils::Grpc::Core::Server
{

class Server final
  : public Generics::ActiveObject,
    public ReferenceCounting::AtomicImpl
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;

private:
  using MethodName = std::string_view;
  using Handler = std::map<MethodName, RpcHandlerInfo>;
  using ServiceName = std::string_view;
  using Handlers = std::map<ServiceName, Handler>;

  using ServerCompletionQueuePtr =
    std::shared_ptr<grpc::ServerCompletionQueue>;
  using ServerCompletionQueues =
    std::vector<ServerCompletionQueuePtr>;
  using Services = std::vector<Service_var>;

public:
  explicit Server(
    const Config& config,
    Logger* logger);

  ~Server() override;

  void activate_object() override;

  void deactivate_object() override;

  void wait_object() override;

  bool active() override;

  template<class RpcHandlerType>
  void register_handler()
  {
    namespace Common = UServerUtils::Grpc::Core::Common;
    
    using Traits = typename RpcHandlerType::Traits;
    using Request = typename Traits::Request;
    using Response = typename Traits::Response;

    const std::string_view method_full_name = Traits::method_name();
    const auto result_split =
      Common::Utils::split(method_full_name, "/");

    if (result_split.size() != 3)
    {
      Stream::Error stream;
      stream << FNS
             << " : not correct format for full method name of method="
             << method_full_name;
      throw Exception(stream);
    }

    auto it = result_split.begin();
    if (*it++ != "")
    {
      Stream::Error stream;
      stream << FNS
             << " : not correct format for full method name of method="
             << method_full_name;
      throw Exception(stream);
    }

    const std::string_view service_full_name(*it++);
    const std::string_view method_name(*it++);

    check_handler_compatibility<RpcHandlerType>(
      std::string(service_full_name),
      std::string(method_name));

    handlers_[service_full_name].emplace(
      method_name,
      RpcHandlerInfo(
        Request::default_instance().GetDescriptor(),
        Response::default_instance().GetDescriptor(),
        Traits::rpc_type,
        [] (Rpc* rpc, CommonContext* common_context) {
          std::unique_ptr<RpcHandler> rpc_handler =
            std::make_unique<RpcHandlerType>();
          rpc_handler->set_rpc(rpc);
          rpc_handler->set_common_context(common_context);
          return rpc_handler;
        },
        method_full_name.data()));
  }

private:
  template <typename RpcHandlerType>
  void check_handler_compatibility(
    const std::string& service_full_name,
    const std::string& method_name)
  {
    using Traits = typename RpcHandlerType::Traits;
    using Request = typename Traits::Request;
    using Response = typename Traits::Response;

    const auto* pool =
      google::protobuf::DescriptorPool::generated_pool();
    if (!pool)
    {
      Stream::Error stream;
      stream << FNS
             << ": google::protobuf::DescriptorPool::generated_pool is null";
      throw Exception(stream);
    }

    const auto* service = pool->FindServiceByName(service_full_name);
    if (!service)
    {
      Stream::Error stream;
      stream << FNS
             << ": Unknow servic = "
             << service_full_name;
      throw Exception(stream);
    }

    const auto* method_descriptor =
      service->FindMethodByName(method_name);
    if (!method_descriptor)
    {
      Stream::Error stream;
      stream << FNS
             << ": Unknow method = "
             << method_name
             << " in service"
             << service_full_name;
      throw Exception(stream);
    }

    const auto* request_type = method_descriptor->input_type();
    if (Request::default_instance().GetDescriptor() != request_type)
    {
      Stream::Error stream;
      stream << FNS
             << ": not correct request type for service="
             << service_full_name
             << " whith method="
             << method_name;
      throw Exception(stream);
    }

    const auto* response_type =
      method_descriptor->output_type();
    if (Response::default_instance().GetDescriptor() != response_type)
    {
      Stream::Error stream;
      stream << FNS
             << ": not correct response type for service="
             << service_full_name
             << " whith method="
             << method_name;
      throw Exception(stream);
    }

    const auto rpc_type = Traits::rpc_type;
    switch (rpc_type)
    {
    case grpc::internal::RpcMethod::NORMAL_RPC:
    {
      if (method_descriptor->client_streaming())
      {
        Stream::Error stream;
        stream << FNS
               << ": incoming type in proto is streaming in NORMAL_RPC mode"
               << " for service="
               << service_full_name
               << " whith method="
               << method_name;
        throw Exception(stream);
      }

      if (method_descriptor->server_streaming())
      {
        Stream::Error stream;
        stream << FNS
               << ": outgoing type in proto is streaming in NORMAL_RPC mode"
               << " for service="
               << service_full_name
               << " whith method="
               << method_name;
        throw Exception(stream);
      }

      break;
    }
    case grpc::internal::RpcMethod::CLIENT_STREAMING:
    {
      if (!method_descriptor->client_streaming())
      {
        Stream::Error stream;
        stream << FNS
               << ": incoming type in proto is not streaming in CLIENT_STREAMING mode"
               << " for service="
               << service_full_name
               << " whith method="
               << method_name;
        throw Exception(stream);
      }

      if (method_descriptor->server_streaming())
      {
        Stream::Error stream;
        stream << FNS
               << ": outgoing type in proto is streaming in CLIENT_STREAMING mode"
               << " for service="
               << service_full_name
               << " whith method="
               << method_name;
        throw Exception(stream);
      }

      break;
    }
    case grpc::internal::RpcMethod::SERVER_STREAMING:
    {
      if (method_descriptor->client_streaming())
      {
        Stream::Error stream;
        stream << FNS
               << ": incoming type in proto is streaming in SERVER_STREAMING mode"
               << " for service="
               << service_full_name
               << " whith method="
               << method_name;
        throw Exception(stream);
      }

      if (!method_descriptor->server_streaming())
      {
        Stream::Error stream;
        stream << FNS
               << ": outgoing type in proto is not streaming in SERVER_STREAMING mode"
               << " for service="
               << service_full_name
               << " whith method="
               << method_name;
        throw Exception(stream);
      }

      break;
    }
    case grpc::internal::RpcMethod::BIDI_STREAMING:
    {
      if (!method_descriptor->client_streaming())
      {
        Stream::Error stream;
        stream << FNS
               << ": incoming type in proto is not streaming in BIDI_STREAMING mode"
               << " for service="
               << service_full_name
               << " whith method="
               << method_name;
        throw Exception(stream);
      }

      if (!method_descriptor->server_streaming())
      {
        Stream::Error stream;
        stream << FNS
               << ": outgoing type in proto is not streaming in BIDI_STREAMING mode"
               << " for service="
               << service_full_name
               << " whith method="
               << method_name;
        throw Exception(stream);
      }

      break;
    }
    }
  }

  void add_channel_args();

  void register_services();

private:
  Config config_;

  Logger_var logger_;

  RpcPoolImpl_var rpc_pool_;

  grpc::ServerBuilder server_builder_;

  ServerCompletionQueues server_completion_queues_;

  Common::SchedulerPtr scheduler_;

  Services services_;

  Handlers handlers_;

  std::unique_ptr<grpc::Server> server_;

  ACTIVE_STATE state_ = AS_NOT_ACTIVE;

  std::mutex state_mutex_;

  std::condition_variable condition_variable_;
};

using Server_var = ReferenceCounting::SmartPtr<Server>;

} // namespace UServerUtils::Grpc::Core::Server

#endif //GRPC_CORE_SERVER_SERVER_H_