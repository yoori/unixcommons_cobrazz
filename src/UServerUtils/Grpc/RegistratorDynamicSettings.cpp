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

  {
    userver::formats::json::ValueBuilder builder(false);
    auto value = builder.ExtractValue();
    docs_map_.Set(
      "USERVER_BAGGAGE_ENABLED",
      std::move(value));
  }

  {
    userver::formats::json::ValueBuilder builder(userver::formats::json::Type::kObject);
    userver::formats::json::ValueBuilder array_builder{userver::formats::json::Type::kArray};
    builder["allowed_keys"] = array_builder.ExtractValue();
    docs_map_.Set(
      "BAGGAGE_SETTINGS",
      builder.ExtractValue());
  }

  {
    userver::formats::json::ValueBuilder builder(false);
    auto value = builder.ExtractValue();
    docs_map_.Set(
      "USERVER_LOG_REQUEST",
      std::move(value));
  }

  {
    userver::formats::json::ValueBuilder builder(false);
    auto value = builder.ExtractValue();
    docs_map_.Set(
      "USERVER_LOG_REQUEST_HEADERS",
      std::move(value));
  }

  {
    userver::formats::json::ValueBuilder builder(false);
    auto value = builder.ExtractValue();
    docs_map_.Set(
      "USERVER_CHECK_AUTH_IN_HANDLERS",
      std::move(value));
  }

  {
    userver::formats::json::ValueBuilder builder(false);
    auto value = builder.ExtractValue();
    docs_map_.Set(
      "USERVER_CANCEL_HANDLE_REQUEST_BY_DEADLINE",
      std::move(value));
  }

  {
    userver::formats::json::ValueBuilder builder(userver::formats::json::Type::kObject);
    auto value = builder.ExtractValue();
    docs_map_.Set(
      "USERVER_RPS_CCONTROL_CUSTOM_STATUS",
      std::move(value));
  }

  {
    userver::formats::json::ValueBuilder builder(userver::formats::json::Type::kObject);
    auto value = builder.ExtractValue();
    docs_map_.Set(
      "USERVER_DUMPS",
      std::move(value));
  }

  {
    userver::formats::json::ValueBuilder builder(userver::formats::json::Type::kObject);
    auto value = builder.ExtractValue();
    docs_map_.Set(
      "USERVER_CACHES",
      std::move(value));
  }

  {
    userver::formats::json::ValueBuilder builder(true);
    auto value = builder.ExtractValue();
    docs_map_.Set(
      "USERVER_HANDLER_STREAM_API_ENABLED",
      std::move(value));
  }
}

RegistratorDynamicSettings::DocsMap&
RegistratorDynamicSettings::docs_map() noexcept
{
  return docs_map_;
}

} // namespace UServerUtils::Grpc