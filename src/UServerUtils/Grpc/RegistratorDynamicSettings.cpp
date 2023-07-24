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
}

RegistratorDynamicSettings::DocsMap&
RegistratorDynamicSettings::docs_map() noexcept
{
  return docs_map_;
}

} // namespace UServerUtils::Grpc