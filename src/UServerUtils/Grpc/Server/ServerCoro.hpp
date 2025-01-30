#ifndef GRPC_SERVER_SERVER_CORO_H_
#define GRPC_SERVER_SERVER_CORO_H_

// STD
#include <optional>
#include <unordered_map>

// USERVER
#include <engine/task/task_processor.hpp>

// THIS
#include <UServerUtils/Component.hpp>
#include <UServerUtils/Grpc/Server/CommonContextCoro.hpp>
#include <UServerUtils/Grpc/Server/ConfigCoro.hpp>
#include <UServerUtils/Grpc/Server/RpcHandlerImpl.hpp>
#include <UServerUtils/Grpc/Server/Server.hpp>
#include <UServerUtils/Grpc/Server/ServiceCoro.hpp>

namespace UServerUtils::Grpc::Server
{

namespace Internal
{

template<class T, class = void>
struct exist_type_request : std::false_type
{
};

template<class T>
struct exist_type_request<T, std::void_t<typename T::Request>> : std::true_type
{
};

template<class T>
constexpr bool exist_type_request_v = exist_type_request<T>::value;

template<class T, class = void>
struct exist_type_response : std::false_type
{
};

template<class T>
struct exist_type_response<T, std::void_t<typename T::Response>> : std::true_type
{
};

template<class T>
constexpr bool exist_type_response_v = exist_type_response<T>::value;

template<class T, class = void>
struct exist_type_handler : std::false_type
{
};

template<class T>
struct exist_type_handler<T, std::void_t<typename T::Handler>> : std::true_type
{
};

template<class T>
constexpr bool exist_type_handler_v = exist_type_handler<T>::value;

} // namespace Internal

class ServerCoro final :
  public UServerUtils::Component,
  public ReferenceCounting::AtomicImpl
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using TaskProcessor = userver::engine::TaskProcessor;

public:
  ServerCoro(
    const ConfigCoro& config,
    Logger* logger);

  const Common::SchedulerPtr& scheduler() const noexcept;

  template<class Service>
  void add_service(
    Service* service,
    TaskProcessor& task_processor,
    const ServiceMode service_mode)
  {
    static_assert(
      Internal::exist_type_request_v<Service>,
      "Service must contain type Request");
    static_assert(
      Internal::exist_type_response_v<Service>,
      "Service must contain type Response");
    static_assert(
      std::is_base_of_v<
        ServiceCoro<
          typename Service::Request,
          typename Service::Response>,
        Service>,
      "Service must be derived from ServiceCoro");
    static_assert(
      Internal::exist_type_handler_v<Service>,
      "Service must contain type Handler");
    static_assert(
      std::is_base_of_v<RpcHandler, typename Service::Handler>,
      "Handler must be derived from RpcHandler");

    if (active())
    {
      Stream::Error stream;
      stream << FNS
             << "already active state";
      throw Exception(stream);
    }

    using Handler = typename Service::Handler;
    using Traits = typename Handler::Traits;

    server_->register_handler<Handler>();

    if (!common_context_coro_)
    {
      Stream::Error stream;
      stream << FNS
             << "common_context_coro is null";
      throw Exception(stream);
    }

    constexpr auto rpc_type = Traits::rpc_type;
    constexpr const char* method_name = Traits::method_name();
    common_context_coro_->add_service(
      method_name,
      service,
      rpc_type,
      service_mode,
      task_processor);
  }

protected:
  ~ServerCoro() override;

private:
  void activate_object_() override;

  void deactivate_object_() override;

private:
  Logger_var logger_;

  Server_var server_;

  CommonContextCoro_var common_context_coro_;
};

using ServerCoro_var = ReferenceCounting::SmartPtr<ServerCoro>;

} // namespace UServerUtils::Grpc::Server

#endif //GRPC_SERVER_SERVER_CORO_H_