// STD
#include <iostream>

// USERVER
#include <ugrpc/client/impl/client_configs.hpp>
#include <ugrpc/server/impl/server_configs.hpp>
#include <userver/formats/json/value_builder.hpp>

// THIS
#include <UServerUtils/Grpc/RegistratorDynamicSettings.hpp>

namespace UServerUtils::Grpc
{

RegistratorDynamicSettings::RegistratorDynamicSettings()
{
  registrate();
}

void RegistratorDynamicSettings::registrate()
{
  {
    userver::formats::json::ValueBuilder builder(true);
    auto value = builder.ExtractValue();
    docs_map_.Set(
      "USERVER_GRPC_CLIENT_ENABLE_DEADLINE_PROPAGATION",
      std::move(value));
  }

  {
    userver::formats::json::ValueBuilder builder(true);
    auto value = builder.ExtractValue();
    docs_map_.Set(
      "USERVER_GRPC_SERVER_CANCEL_TASK_BY_DEADLINE",
      std::move(value));
  }

  // next block is required only if we will try to build userver shared in future again ...
  /*
  {
    userver::formats::json::ValueBuilder allowed_keys(userver::formats::json::Type::kArray);
    userver::formats::json::ValueBuilder builder(userver::formats::json::Type::kObject);
    builder["allowed_keys"] = allowed_keys;
    auto value = builder.ExtractValue();
    docs_map_.Set("BAGGAGE_SETTINGS", std::move(value));
  }

  {
    userver::formats::json::ValueBuilder builder(false);
    auto value = builder.ExtractValue();
    docs_map_.Set("USERVER_BAGGAGE_ENABLED", std::move(value));
  }

  {
    userver::formats::json::ValueBuilder builder(userver::formats::json::Type::kObject);
    auto value = builder.ExtractValue();
    docs_map_.Set("USERVER_CACHES", std::move(value));
  }

  {
    userver::formats::json::ValueBuilder builder(userver::formats::json::Type::kObject);
    auto value = builder.ExtractValue();
    docs_map_.Set("USERVER_LRU_CACHES", std::move(value));
  }

  {
    userver::formats::json::ValueBuilder builder(1000);
    auto value = builder.ExtractValue();
    docs_map_.Set("HTTP_CLIENT_CONNECTION_POOL_SIZE", std::move(value));
  }

  // http factory default traits
  {
    userver::formats::json::ValueBuilder builder(userver::formats::json::Type::kObject);
    builder["cancel-request"] = userver::formats::json::ValueBuilder(false).ExtractValue();
    builder["update-timeout"] = userver::formats::json::ValueBuilder(false).ExtractValue();
    auto value = builder.ExtractValue();
    docs_map_.Set("HTTP_CLIENT_ENFORCE_TASK_DEADLINE", std::move(value));
  }

  {
    userver::formats::json::ValueBuilder builder("");
    auto value = builder.ExtractValue();
    docs_map_.Set("USERVER_HTTP_PROXY", std::move(value));
  }

  {
    userver::formats::json::ValueBuilder builder(userver::formats::json::Type::kObject);
    builder["max-size"] = userver::formats::json::ValueBuilder(100).ExtractValue();
    builder["token-update-interval-ms"] = userver::formats::json::ValueBuilder(0).ExtractValue();
    auto value = builder.ExtractValue();
    docs_map_.Set("HTTP_CLIENT_CONNECT_THROTTLE", std::move(value));
  }

  {
    userver::formats::json::ValueBuilder builder(userver::formats::json::Type::kObject);
    builder["names"] = userver::formats::json::ValueBuilder(userver::formats::json::Type::kArray).ExtractValue();
    builder["prefixes"] = userver::formats::json::ValueBuilder(userver::formats::json::Type::kArray).ExtractValue();
    auto value = builder.ExtractValue();
    docs_map_.Set("USERVER_NO_LOG_SPANS", std::move(value));
  }

  {
    userver::formats::json::ValueBuilder builder(userver::formats::json::Type::kObject);
    builder["force-disabled"] = userver::formats::json::ValueBuilder(userver::formats::json::Type::kArray).ExtractValue();
    builder["force-enabled"] = userver::formats::json::ValueBuilder(userver::formats::json::Type::kArray).ExtractValue();
    auto value = builder.ExtractValue();
    docs_map_.Set("USERVER_LOG_DYNAMIC_DEBUG", std::move(value));
  }

  {
    userver::formats::json::ValueBuilder wait_queue_overload_builder(userver::formats::json::Type::kObject);
    wait_queue_overload_builder["action"] = userver::formats::json::ValueBuilder("ignore").ExtractValue();
    wait_queue_overload_builder["length_limit"] = userver::formats::json::ValueBuilder(5000).ExtractValue();
    wait_queue_overload_builder["time_limit_us"] = userver::formats::json::ValueBuilder(3000).ExtractValue();

    userver::formats::json::ValueBuilder default_task_processor_builder(userver::formats::json::Type::kObject);
    default_task_processor_builder["wait_queue_overload"] = wait_queue_overload_builder.ExtractValue();

    userver::formats::json::ValueBuilder builder(userver::formats::json::Type::kObject);
    builder["default-service"] = default_task_processor_builder.ExtractValue();

    auto value = builder.ExtractValue();
    docs_map_.Set("USERVER_TASK_PROCESSOR_QOS", std::move(value));
  }

  {
    userver::formats::json::ValueBuilder wait_queue_overload_builder(userver::formats::json::Type::kObject);

    {
      wait_queue_overload_builder["action"] = userver::formats::json::ValueBuilder("ignore").ExtractValue();
      wait_queue_overload_builder["length_limit"] = userver::formats::json::ValueBuilder(5000).ExtractValue();
      wait_queue_overload_builder["time_limit_us"] = userver::formats::json::ValueBuilder(3000).ExtractValue();
    }

    wait_queue_overload_builder["wait_queue_overload"] = wait_queue_overload_builder.ExtractValue();

    userver::formats::json::ValueBuilder default_task_processor_builder(userver::formats::json::Type::kObject);
    default_task_processor_builder["wait_queue_overload"] = wait_queue_overload_builder.ExtractValue();

    docs_map_.Set("default-service", std::move(default_task_processor_builder.ExtractValue()));
  }
  */
}

RegistratorDynamicSettings::DocsMap&
RegistratorDynamicSettings::docs_map() noexcept
{
  return docs_map_;
}

} // namespace UServerUtils::Grpc
