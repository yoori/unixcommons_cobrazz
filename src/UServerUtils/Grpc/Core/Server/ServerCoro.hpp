#ifndef GRPC_CORE_SERVER_SERVER_CORO_H_
#define GRPC_CORE_SERVER_SERVER_CORO_H_

// STD
#include <optional>
#include <unordered_map>

// USERVER
#include <engine/task/task_processor.hpp>

// THIS
#include <UServerUtils/Grpc/Component.hpp>
#include <UServerUtils/Grpc/Core/Server/CommonContextCoro.hpp>
#include <UServerUtils/Grpc/Core/Server/ConfigCoro.hpp>
#include <UServerUtils/Grpc/Core/Server/RpcHandlerImpl.hpp>
#include <UServerUtils/Grpc/Core/Server/Server.hpp>
#include <UServerUtils/Grpc/Core/Server/ServiceCoro.hpp>

namespace UServerUtils::Grpc::Core::Server
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
  public UServerUtils::Grpc::Component,
  public ReferenceCounting::AtomicImpl
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using TaskProcessor = userver::engine::TaskProcessor;
  using MethodName = std::string;

public:
  ServerCoro(
    const ConfigCoro& config,
    Logger* logger);

  void activate_object() override;

  void deactivate_object() override;

  void wait_object() override;

  bool active() override;

  template<class Service>
  void add_service(
    Service* service,
    TaskProcessor& task_processor,
    const std::optional<std::size_t> number_coro = {})
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

    {
      std::unique_lock<std::mutex> lock(state_mutex_);
      if (state_ != AS_NOT_ACTIVE)
      {
        Stream::Error stream;
        stream << FNS
               << ": state not equal AS_NOT_ACTIVE";
        throw Exception(stream);
      }
    }

    using Request = typename Service::Request;
    using Response = typename Service::Response;
    using Handler = typename Service::Handler;
    using Traits = typename Handler::Traits;

    server_->register_handler<Handler>();

    if (!common_context_coro_)
    {
      Stream::Error stream;
      stream << FNS
             << ": common_context_coro is null";
      throw Exception(stream);
    }

    constexpr auto rpc_type = Traits::rpc_type;
    constexpr const char* method_name = Traits::method_name();
    common_context_coro_->add_service(
      method_name,
      service,
      rpc_type,
      task_processor,
      number_coro);
  }

protected:
  ~ServerCoro() override;

private:
  Logger_var logger_;

  Server_var server_;

  CommonContextCoro_var common_context_coro_;

  ACTIVE_STATE state_ = AS_NOT_ACTIVE;

  std::mutex state_mutex_;

  std::condition_variable condition_variable_;
};

using ServerCoro_var = ReferenceCounting::SmartPtr<ServerCoro>;

} // namespace UServerUtils::Grpc::Core::Server

#endif //GRPC_CORE_SERVER_SERVER_CORO_H_