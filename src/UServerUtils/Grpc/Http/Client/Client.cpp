// THIS
#include <UServerUtils/Grpc/Http/Client/Client.hpp>

namespace UServerUtils::Http::Client
{

Client::Client(
  const ClientConfig& config,
  userver::engine::TaskProcessor& task_processor)
  : impl_(std::make_shared<Impl>(
      config,
      task_processor,
      userver::clients::http::impl::PluginPipeline{{}})),
    task_processor_(task_processor)
{
}

Request Client::create_request()
{
  return Request{impl_->CreateRequest(), task_processor_};
}

Request Client::create_not_signed_request()
{
  return Request{impl_->CreateNotSignedRequest(), task_processor_};
}

void Client::reset_user_agent(const std::optional<std::string> user_agent)
{
  impl_->ResetUserAgent(user_agent);
}

std::string Client::get_proxy() const
{
  return impl_->GetProxy();
}

} // namespace UServerUtils::Http::Client