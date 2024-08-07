// THIS
#include <UServerUtils/Http/Client/Client.hpp>
#include <UServerUtils/Utils.hpp>

namespace UServerUtils::Http::Client
{

Client::Client(
  const ClientConfig& config,
  userver::engine::TaskProcessor& task_processor)
  : task_processor_(task_processor)
{
  impl_ = UServerUtils::Utils::run_in_coro(
    task_processor_,
    UServerUtils::Utils::Importance::kCritical,
    {},
    [config, &task_processor] () {
      return std::make_shared<Impl>(
        config,
        task_processor,
        userver::clients::http::impl::PluginPipeline{{}});
    });
}

Client::~Client()
{
  try
  {
    UServerUtils::Utils::run_in_coro(
      task_processor_,
      UServerUtils::Utils::Importance::kCritical,
      {},
      [impl = std::move(impl_)] () mutable {
        impl.reset();
      });
  }
  catch (...)
  {
  }
}

Request Client::create_request()
{
  return UServerUtils::Utils::run_in_coro(
    task_processor_,
    UServerUtils::Utils::Importance::kCritical,
    {},
    [impl = impl_, task_processor = &task_processor_] () {
      return Request{impl->CreateRequest(), *task_processor};
    });
}

Request Client::create_not_signed_request()
{
  return UServerUtils::Utils::run_in_coro(
    task_processor_,
    UServerUtils::Utils::Importance::kCritical,
    {},
    [impl = impl_, task_processor = &task_processor_] () {
      return Request{impl->CreateNotSignedRequest(), *task_processor};
    });
}

void Client::reset_user_agent(std::optional<std::string> user_agent)
{
  impl_->ResetUserAgent(std::move(user_agent));
}

std::string Client::get_proxy() const
{
  return impl_->GetProxy();
}

} // namespace UServerUtils::Http::Client