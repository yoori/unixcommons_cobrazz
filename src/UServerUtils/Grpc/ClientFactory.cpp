// THIS
#include "ClientFactory.hpp"

namespace UServerUtils::Grpc
{

GrpcClientFactory::GrpcClientFactory(
  GrpcClientFactoryConfig&& config,
  TaskProcessor& channel_task_processor,
  CompletionQueue& queue,
  StatisticsStorage& statistics_storage)
{
  ClientFactoryConfig client_config;
  client_config.channel_args = std::move(config.channel_args);
  client_config.channel_count = config.channel_count;
  client_config.credentials = grpc::InsecureChannelCredentials();
  client_config.native_log_level = userver::logging::Level::kError;

  client_factory_ =
    std::make_unique<ClientFactory>(
      std::move(client_config),
      channel_task_processor,
      queue,
      statistics_storage);
}

} // namespace UServerUtils::Grpc