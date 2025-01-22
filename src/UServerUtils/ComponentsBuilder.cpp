// STD
#include <algorithm>
#include <iterator>
#include <sstream>

// THIS
#include <UServerUtils/Statistics/MemoryStatisticsProvider.hpp>
#include <UServerUtils/ComponentsBuilder.hpp>

namespace UServerUtils
{

namespace Aspect
{

const char Builder[] = "BUILDER";

} // namespace Aspect

ComponentsBuilder::ComponentsBuilder(
  const std::optional<StatisticsProviderInfo>& statistics_provider_info)
  : statistics_storage_(new StatisticsStorage())
{
  if (statistics_provider_info.has_value())
  {
    statistics_provider_ = statistics_provider_info->statistics_provider;
    statistics_prefix_ = statistics_provider_info->statistics_prefix;
  }
}

ComponentsBuilder::StatisticsStorage&
ComponentsBuilder::get_statistics_storage()
{
  if (!statistics_storage_)
  {
    Stream::Error stream;
    stream << FNS
           << "statistics storage is null";
    throw Exception(stream);
  }

  return *statistics_storage_;
}

void ComponentsBuilder::add_grpc_userver_server(
  GrpcUserverServerBuilderPtr&& builder)
{
  auto grpc_builder = std::move(builder);
  if (!grpc_builder)
  {
    Stream::Error stream;
    stream << FNS
           << "builder is null";
    throw Exception(stream);
  }

  auto server_info = grpc_builder->build();
  auto& server = server_info.server;
  auto& services = server_info.services;
  auto& middlewares_list = server_info.middlewares_list;

  add_component_cash(server);
  grpc_userver_servers_.emplace_back(std::move(server));
  std::move(
    std::begin(middlewares_list),
    std::end(middlewares_list),
    std::back_inserter(middlewares_list_));

  for (auto& service : services)
  {
    add_component(service);
  }
}

void ComponentsBuilder::add_grpc_cobrazz_server(
  GrpcCobrazzServerBuilderPtr&& builder)
{
  auto grpc_builder = std::move(builder);
  if (!grpc_builder)
  {
    Stream::Error stream;
    stream << FNS
           << "builder is null";
    throw Exception(stream);
  }

  auto server_info = grpc_builder->build();
  auto& server = server_info.server;
  auto& services = server_info.services;

  add_component_cash(server);
  grpc_cobrazz_servers_.emplace_back(
    std::move(server));

  for (auto& service : services)
  {
    add_component(service);
  }
}

void ComponentsBuilder::add_http_server(
  HttpServerBuilderPtr&& builder)
{
  auto http_builder = std::move(builder);
  if (!http_builder)
  {
    Stream::Error stream;
    stream << FNS
           << "builder is null";
    throw Exception(stream);
  }

  auto server_info = http_builder->build();
  auto& server = server_info.http_server;
  auto& handlers = server_info.http_handlers;

  add_component_cash(server);
  http_servers_.emplace_back(std::move(server));

  for (auto& handler : handlers)
  {
    add_component(handler);
  }
}

ComponentsBuilder::GrpcUserverClientFactory_var
ComponentsBuilder::add_grpc_userver_client_factory(
  GrpcUserverClientFactoryConfig&& config,
  TaskProcessor& channel_task_processor,
  const CompletionQueuePoolBasePtr& completion_qeue_pool,
  const MiddlewareFactories& middleware_factories)
{
  bool pool_exist = false;
  for (auto& pool : completion_qeue_pool_list_)
  {
    if (pool == completion_qeue_pool)
    {
      pool_exist = true;
      break;
    }
  }

  if (!pool_exist)
  {
    completion_qeue_pool_list_.emplace_back(
      completion_qeue_pool);
  }

  GrpcUserverClientFactory_var grpc_client_factory(
    new GrpcUserverClientFactory(
      std::move(config),
      channel_task_processor,
      *statistics_storage_,
      completion_qeue_pool,
      middleware_factories));

  add_component(grpc_client_factory.in());

  return grpc_client_factory;
}

void ComponentsBuilder::add_component(Component* component)
{
  if (check_component_cash(component))
  {
    Stream::Error stream;
    stream << FNS
           << "component already exist";
    throw Exception(stream);
  }

  add_component_cash(component);
  components_.emplace_back(
    Component_var(ReferenceCounting::add_ref(component)));
}

void ComponentsBuilder::add_user_component(
  const std::string& name,
  Component* component)
{
  if (name_to_user_component_.count(name) == 1)
  {
    Stream::Error stream;
    stream << FNS
           << "user component with name="
           << name
           << " already exist";
    throw Exception(stream);
  }

  add_component(component);
  name_to_user_component_.try_emplace(
    name,
    Component_var(ReferenceCounting::add_ref(component)));
}

bool ComponentsBuilder::check_component_cash(
  Component* component)
{
  const auto it = cash_existing_component_.find(
    reinterpret_cast<std::uintptr_t>(component));
  if (it == cash_existing_component_.end())
  {
    return false;
  }
  else
  {
    return true;
  }
}

void ComponentsBuilder::add_component_cash(
  Component* component)
{
  cash_existing_component_.emplace(
    reinterpret_cast<std::uintptr_t>(component));
}

ComponentsBuilder::ComponentsInfo
ComponentsBuilder::build()
{
  using Entry = userver::utils::statistics::Entry;
  using MemoryStatisticsProvider =
    UServerUtils::Statistics::MemoryStatisticsProvider;

  std::move(
    std::begin(grpc_userver_servers_),
    std::end(grpc_userver_servers_),
    std::back_inserter(components_));

  std::move(
    std::begin(grpc_cobrazz_servers_),
    std::end(grpc_cobrazz_servers_),
    std::back_inserter(components_));

  std::move(
    std::begin(http_servers_),
    std::end(http_servers_),
    std::back_inserter(components_));

  StatisticsHolders statistics_holders;
  if (statistics_provider_)
  {
    auto entry = statistics_storage_->RegisterWriter(
      statistics_prefix_,
      [statistics_provider = std::move(statistics_provider_)] (
        userver::utils::statistics::Writer& writer) {
        try
        {
          statistics_provider->write(writer);
        }
        catch (...)
        {
        }
      },
      {});

    StatisticsHolderPtr statistics_holder = std::make_unique<Entry>(
      std::move(entry));
    statistics_holders.emplace_back(std::move(statistics_holder));
  }

  auto memory_statistics_provider = std::make_shared<MemoryStatisticsProvider>();
  auto entry = statistics_storage_->RegisterWriter(
    memory_statistics_provider->name(),
    [memory_statistics_provider = std::move(memory_statistics_provider)] (
      userver::utils::statistics::Writer& writer) {
      try
      {
        memory_statistics_provider->write(writer);
      }
      catch (...)
      {
      }
    },
    {});
  statistics_holders.emplace_back(
    std::make_unique<Entry>(
      std::move(entry)));

  ComponentsInfo components_info;
  components_info.statistics_storage = std::move(statistics_storage_);
  components_info.statistics_holders = std::move(statistics_holders);
  components_info.completion_qeue_pool_list = std::move(completion_qeue_pool_list_);
  components_info.components = std::move(components_);
  components_info.name_to_user_component = std::move(name_to_user_component_);
  components_info.middlewares_list = std::move(middlewares_list_);

  return components_info;
}

} // namespace UServerUtils