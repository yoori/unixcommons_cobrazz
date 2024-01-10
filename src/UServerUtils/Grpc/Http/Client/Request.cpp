// STD
#include <iostream>

//THIS
#include <UServerUtils/Grpc/Http/Client/Request.hpp>
#include <UServerUtils/Grpc/Utils.hpp>

namespace UServerUtils::Http::Client
{

Request::Request(
  const Impl& impl,
  TaskProcessor& task_processor)
  : task_processor_(task_processor),
    impl_(std::make_shared<Impl>(impl))
{
}

Request::Request(
  Impl&& impl,
  TaskProcessor& task_processor)
  : task_processor_(task_processor),
    impl_(std::make_shared<Impl>(std::move(impl)))
{
}

Request& Request::method(const HttpMethod method) &
{
  impl_->method(method);
  return *this;
}

Request Request::method(const HttpMethod method) &&
{
  return Request{
    std::move(*impl_).method(method),
    task_processor_};
}

Request& Request::get() &
{
  return *this;
}

Request Request::get() &&
{
  return Request{
    std::move(*impl_).get(),
    task_processor_};
}

Request& Request::get(const std::string& url) &
{
  impl_->get(url);
  return *this;
}

Request Request::get(const std::string& url) &&
{
  return Request{
    std::move(*impl_).get(url),
    task_processor_};
}

Request& Request::head() &
{
  impl_->head();
  return *this;
}

Request Request::head() &&
{
  return Request{
    std::move(*impl_).head(),
    task_processor_};
}

Request& Request::head(const std::string& url) &
{
  impl_->head(url);
  return *this;
}

Request Request::head(const std::string& url) &&
{
  return Request{
    std::move(*impl_).head(url),
    task_processor_};
}

Request& Request::post() &
{
  impl_->post();
  return *this;
}

Request Request::post() &&
{
  return Request{
    std::move(*impl_).post(),
    task_processor_};
}

Request& Request::post(
  const std::string& url,
  std::string&& data) &
{
  impl_->post(url, std::move(data));
  return *this;
}

Request Request::post(
  const std::string& url,
  std::string&& data) &&
{
  return Request{
    std::move(*impl_).post(url, std::move(data)),
    task_processor_};
}

Request& Request::post(
  const std::string& url,
  const Form& form) &
{
  impl_->post(url, form);
  return *this;
}

Request Request::post(
  const std::string& url,
  const Form& form) &&
{
  return Request{
    std::move(*impl_).post(url, form),
    task_processor_};
}

Request& Request::put() &
{
  impl_->put();
  return *this;
}

Request Request::put() &&
{
  return Request{
    std::move(*impl_).put(),
    task_processor_};
}

Request& Request::put(
  const std::string& url,
  std::string&& data) &
{
  impl_->put(url, std::move(data));
  return *this;
}

Request Request::put(
  const std::string& url,
  std::string&& data) &&
{
  return Request{
    std::move(*impl_).put(url, std::move(data)),
    task_processor_};
}

Request& Request::patch() &
{
  impl_->patch();
  return *this;
}

Request Request::patch() &&
{
  return Request{
    std::move(*impl_).patch(),
    task_processor_};
}

Request& Request::patch(
  const std::string& url,
  std::string&& data) &
{
  impl_->patch(url, std::move(data));
  return *this;
}

Request Request::patch(
  const std::string& url,
  std::string&& data) &&
{
  return Request{
    std::move(*impl_).patch(url, std::move(data)),
    task_processor_};
}

Request& Request::delete_method() &
{
  impl_->delete_method();
  return *this;
}

Request Request::delete_method() &&
{
  return Request{
    std::move(*impl_).delete_method(),
    task_processor_};
}

Request& Request::delete_method(const std::string& url) &
{
  impl_->delete_method(url);
  return *this;
}

Request Request::delete_method(const std::string& url) &&
{
  return Request{
    std::move(*impl_).delete_method(url),
    task_processor_};
}

Request& Request::delete_method(
  const std::string& url,
  std::string&& data) &
{
  impl_->delete_method(url, std::move(data));
  return *this;
}

Request Request::delete_method(
  const std::string& url,
  std::string&& data) &&
{
  return Request{
    std::move(*impl_).delete_method(url, std::move(data)),
    task_processor_};
}

Request& Request::set_custom_http_request_method(std::string&& method) &
{
  impl_->set_custom_http_request_method(std::move(method));
  return *this;
}

Request Request::set_custom_http_request_method(std::string&& method) &&
{
  return Request{
    std::move(*impl_).set_custom_http_request_method(std::move(method)),
    task_processor_};
}

Request& Request::url(const std::string& url) &
{
  impl_->url(url);
  return *this;
}

Request Request::url(const std::string& url) &&
{
  return Request{
    std::move(*impl_).url(url),
    task_processor_};
}

Request& Request::data(std::string&& data) &
{
  impl_->data(std::move(data));
  return *this;
}

Request Request::data(std::string&& data) &&
{
  return Request{
    std::move(*impl_).data(std::move(data)),
    task_processor_};
}

Request& Request::form(const Form& form) &
{
  impl_->form(form);
  return *this;
}
Request Request::form(const Form& form) &&
{
  return Request{
    std::move(*impl_).form(form),
    task_processor_};
}

Request& Request::headers(const Headers& headers) &
{
  impl_->headers(headers);
  return *this;
}

Request Request::headers(const Headers& headers) &&
{
  return Request{
    std::move(*impl_).headers(headers),
    task_processor_};
}

Request& Request::headers(
  const std::initializer_list<
    std::pair<std::string_view, std::string_view>>& headers) &
{
  impl_->headers(headers);
  return *this;
}

Request Request::headers(
  const std::initializer_list<
    std::pair<std::string_view, std::string_view>>& headers) &&
{
  return Request{
    std::move(*impl_).headers(headers),
    task_processor_};
}

Request& Request::proxy_headers(const Headers& headers) &
{
  impl_->proxy_headers(headers);
  return *this;
}

Request Request::proxy_headers(const Headers& headers) &&
{
  return Request{
    std::move(*impl_).proxy_headers(headers),
    task_processor_};
}

Request& Request::proxy_headers(
  const std::initializer_list<
    std::pair<std::string_view, std::string_view>>& headers) &
{
  impl_->proxy_headers(headers);
  return *this;
}

Request Request::proxy_headers(
  const std::initializer_list<
    std::pair<std::string_view, std::string_view>>& headers) &&
{
  return Request{
    std::move(*impl_).proxy_headers(headers),
    task_processor_};
}

Request& Request::user_agent(const std::string& value) &
{
  impl_->user_agent(value);
  return *this;
}

Request Request::user_agent(const std::string& value) &&
{
  return Request{
    std::move(*impl_).user_agent(value),
    task_processor_};
}

Request& Request::proxy(const std::string& value) &
{
  impl_->proxy(value);
  return *this;
}

Request Request::proxy(const std::string& value) &&
{
  return Request{
    std::move(*impl_).proxy(value),
    task_processor_};
}

Request& Request::proxy_auth_type(const ProxyAuthType value) &
{
  impl_->proxy_auth_type(value);
  return *this;
}

Request Request::proxy_auth_type(const ProxyAuthType value) &&
{
  return Request{
    std::move(*impl_).proxy_auth_type(value),
    task_processor_};
}

Request& Request::cookies(const Cookies& cookies) &
{
  impl_->cookies(cookies);
  return *this;
}

Request Request::cookies(const Cookies& cookies) &&
{
  return Request{
    std::move(*impl_).cookies(cookies),
    task_processor_};
}

Request& Request::cookies(
  const std::unordered_map<std::string, std::string>& cookies) &
{
  impl_->cookies(cookies);
  return *this;
}

Request Request::cookies(
  const std::unordered_map<std::string, std::string>& cookies) &&
{
  return Request{
    std::move(*impl_).cookies(cookies),
    task_processor_};
}

Request& Request::follow_redirects(bool follow) &
{
  impl_->follow_redirects(follow);
  return *this;
}

Request Request::follow_redirects(bool follow) &&
{
  return Request{
    std::move(*impl_).follow_redirects(follow),
    task_processor_};
}

Request& Request::timeout(long timeout_ms) &
{
  impl_->timeout(timeout_ms);
  return *this;
}

Request Request::timeout(long timeout_ms) &&
{
  return Request{
    std::move(*impl_).timeout(timeout_ms),
    task_processor_};
}

Request& Request::timeout(const std::chrono::milliseconds& timeout_ms) &
{
  impl_->timeout(timeout_ms);
  return *this;
}

Request Request::timeout(const std::chrono::milliseconds& timeout_ms) &&
{
  return Request{
    std::move(*impl_).timeout(timeout_ms),
    task_processor_};
}

Request& Request::verify(bool verify) &
{
  impl_->verify(verify);
  return *this;
}

Request Request::verify(bool verify) &&
{
  return Request{
    std::move(*impl_).verify(verify),
    task_processor_};
}

Request& Request::ca_info(const std::string& file_path) &
{
  impl_->ca_info(file_path);
  return *this;
}

Request Request::ca_info(const std::string& file_path) &&
{
  return Request{
    std::move(*impl_).ca_info(file_path),
    task_processor_};
}

Request& Request::ca(const Certificate& cert) &
{
  impl_->ca(cert);
  return *this;
}

Request Request::ca(const Certificate& cert) &&
{
  return Request{
    std::move(*impl_).ca(cert),
    task_processor_};
}

Request& Request::crl_file(const std::string& file_path) &
{
  impl_->crl_file(file_path);
  return *this;
}

Request Request::crl_file(const std::string& file_path) &&
{
  return Request{
    std::move(*impl_).crl_file(file_path),
    task_processor_};
}

Request& Request::client_key_cert(
  const PrivateKey& pkey,
  const Certificate& cert) &
{
  impl_->client_key_cert(pkey, cert);
  return *this;
}

Request Request::client_key_cert(
  const PrivateKey& pkey,
  const Certificate& cert) &&
{
  return Request{
    std::move(*impl_).client_key_cert(pkey, cert),
    task_processor_};
}

Request& Request::http_version(const HttpVersion version) &
{
  impl_->http_version(version);
  return *this;
}

Request Request::http_version(const HttpVersion version) &&
{
  return Request{
    std::move(*impl_).http_version(version),
    task_processor_};
}

Request& Request::retry(
  const short retries,
  const bool on_fails) &
{
  impl_->retry(retries, on_fails);
  return *this;
}

Request Request::retry(
  const short retries,
  const bool on_fails) &&
{
  return Request{
    std::move(*impl_).retry(retries, on_fails),
    task_processor_};
}

Request& Request::unix_socket_path(const std::string& path) &
{
  impl_->unix_socket_path(path);
  return *this;
}

Request Request::unix_socket_path(const std::string& path) &&
{
  return Request{
    std::move(*impl_).unix_socket_path(path),
    task_processor_};
}

Request& Request::connect_to(const ConnectTo& connect_to) &
{
  impl_->connect_to(connect_to);
  return *this;
}

Request Request::connect_to(const ConnectTo& connect_to) &&
{
  return Request{
    std::move(*impl_).connect_to(connect_to),
    task_processor_};
}

Request& Request::set_logged_url(const std::string& url) &
{
  impl_->SetLoggedUrl(url);
  return *this;
}

Request Request::set_logged_url(const std::string& url) &&
{
  return Request{
    std::move(*impl_).SetLoggedUrl(url),
    task_processor_};
}

Request& Request::set_destination_metric_name(const std::string& destination) &
{
  impl_->SetDestinationMetricName(destination);
  return *this;
}

Request Request::set_destination_metric_name(const std::string& destination) &&
{
  return Request{
    std::move(*impl_).SetDestinationMetricName(destination),
    task_processor_};
}

Request& Request::set_allowed_urls_extra(const std::vector<std::string>& urls) &
{
  impl_->SetAllowedUrlsExtra(urls);
  return *this;
}

Request Request::set_allowed_urls_extra(const std::vector<std::string>& urls) &&
{
  return Request{
    std::move(*impl_).SetAllowedUrlsExtra(urls),
    task_processor_};
}

Request& Request::disable_reply_decoding() &
{
  impl_->DisableReplyDecoding();
  return *this;
}

Request Request::disable_reply_decoding() &&
{
  return Request{
    std::move(*impl_).DisableReplyDecoding(),
    task_processor_};
}

Request& Request::enable_add_client_timeout_header() &
{
  impl_->EnableAddClientTimeoutHeader();
  return *this;
}

Request Request::enable_add_client_timeout_header() &&
{
  return Request{
    std::move(*impl_).EnableAddClientTimeoutHeader(),
    task_processor_};
}

Request& Request::disable_add_client_timeout_header() &
{
  impl_->DisableAddClientTimeoutHeader();
  return *this;
}

Request Request::disable_add_client_timeout_header() &&
{
  return Request{
    std::move(*impl_).DisableAddClientTimeoutHeader(),
    task_processor_};
}

std::shared_ptr<Response> Request::perform(
  const SourceLocation& location)
{
  return UServerUtils::Grpc::Utils::run_in_coro(
    task_processor_,
    UServerUtils::Grpc::Utils::Importance::kNormal,
    {},
    [location = location, impl = impl_] () {
      return impl->perform(location);
    });
}

} // namespace UServerUtils::Http::Client