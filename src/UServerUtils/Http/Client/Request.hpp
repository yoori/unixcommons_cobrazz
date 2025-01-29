#ifndef USERVER_HTTP_CLIENT_REQUEST_HPP
#define USERVER_HTTP_CLIENT_REQUEST_HPP

// STD
#include <memory>

// USERVER
#include <engine/task/task_processor.hpp>
#include <userver/clients/http/request.hpp>
#include <userver/clients/http/response.hpp>

namespace UServerUtils::Http::Client
{

using Certificate = userver::crypto::Certificate;
using ConnectTo = userver::clients::http::ConnectTo;
using Cookies = userver::clients::http::Request::Cookies;
using Form = userver::clients::http::Form;
using Headers = userver::clients::http::Headers;
using HttpMethod = userver::clients::http::HttpMethod;
using HttpVersion = userver::clients::http::HttpVersion;
using HttpAuthType = userver::clients::http::HttpAuthType;
using CancellationPolicy = userver::clients::http::CancellationPolicy;
using PrivateKey = userver::crypto::PrivateKey;
using ProxyAuthType = userver::clients::http::ProxyAuthType;
using Response = userver::clients::http::Response;
using SourceLocation = userver::utils::impl::SourceLocation;

class Request final
{
private:
  using Impl = userver::clients::http::Request;
  using ImplPtr = std::shared_ptr<Impl>;
  using TaskProcessor = userver::engine::TaskProcessor;

public:
  /// Specifies method
  Request& method(const HttpMethod method) &;
  Request method(const HttpMethod method) &&;
  /// GET request
  Request& get() &;
  Request get() &&;
  /// GET request with url
  Request& get(const std::string& url) &;
  Request get(const std::string& url) &&;
  /// HEAD request
  Request& head() &;
  Request head() &&;
  /// HEAD request with url
  Request& head(const std::string& url) &;
  Request head(const std::string& url) &&;
  /// POST request
  Request& post() &;
  Request post() &&;
  /// POST request with url and data
  Request& post(
    const std::string& url,
    std::string&& data = {}) &;
  Request post(
    const std::string& url,
    std::string&& data = {}) &&;
  /// POST request with url and multipart/form-data
  Request& post(
    const std::string& url,
    Form&& form) &;
  Request post(
    const std::string& url,
    Form&& form) &&;
  /// PUT request
  Request& put() &;
  Request put() &&;
  /// PUT request with url and data
  Request& put(
    const std::string& url,
    std::string&& data = {}) &;
  Request put(
    const std::string& url,
    std::string&& data = {}) &&;

  /// PATCH request
  Request& patch() &;
  Request patch() &&;
  /// PATCH request with url and data
  Request& patch(
    const std::string& url,
    std::string&& data = {}) &;
  Request patch(
    const std::string& url,
    std::string&& data = {}) &&;

  /// DELETE request
  Request& delete_method() &;
  Request delete_method() &&;
  /// DELETE request with url
  Request& delete_method(const std::string& url) &;
  Request delete_method(const std::string& url) &&;
  /// DELETE request with url and data
  Request& delete_method(
    const std::string& url,
    std::string&& data) &;
  Request delete_method(
    const std::string& url,
    std::string&& data) &&;

  /// Set custom request method. Only replaces name of the HTTP method
  Request& set_custom_http_request_method(std::string&& method) &;
  Request set_custom_http_request_method(std::string&& method) &&;

  /// url if you don't specify request type with url
  Request& url(const std::string& url) &;
  Request url(const std::string& url) &&;
  /// data for POST request
  Request& data(std::string&& data) &;
  Request data(std::string&& data) &&;
  /// form for POST request
  Request& form(Form&& form) &;
  Request form(Form&& form) &&;
  /// Headers for request as map
  Request& headers(const Headers& headers) &;
  Request headers(const Headers& headers) &&;
  /// Headers for request as list
  Request& headers(
    const std::initializer_list<
      std::pair<std::string_view, std::string_view>>& headers) &;
  Request headers(
    const std::initializer_list<
      std::pair<std::string_view, std::string_view>>& headers) &&;
  /// Sets http auth type to use.
  Request& http_auth_type(
    const HttpAuthType value,
    const bool auth_only,
    const std::string_view user,
    const std::string_view password) &;
  Request http_auth_type(
    const HttpAuthType value,
    const bool auth_only,
    const std::string_view user,
    const std::string_view password) &&;
  /// Proxy headers for request as map
  Request& proxy_headers(const Headers& headers) &;
  Request proxy_headers(const Headers& headers) &&;
  /// Proxy headers for request as list
  Request& proxy_headers(
    const std::initializer_list<
      std::pair<std::string_view, std::string_view>>& headers) &;
  Request proxy_headers(
    const std::initializer_list<
      std::pair<std::string_view, std::string_view>>& headers) &&;
  /// Sets the User-Agent header
  Request& user_agent(const std::string& value) &;
  Request user_agent(const std::string& value) &&;
  /// Sets proxy to use. Example: [::1]:1080
  Request& proxy(const std::string& value) &;
  Request proxy(const std::string& value) &&;
  /// Sets proxy auth type to use.
  Request& proxy_auth_type(const ProxyAuthType value) &;
  Request proxy_auth_type(const ProxyAuthType value) &&;
  /// Cookies for request as HashDos-safe map
  Request& cookies(const Cookies& cookies) &;
  Request cookies(const Cookies& cookies) &&;
  /// Cookies for request as map
  Request& cookies(
    const std::unordered_map<std::string, std::string>& cookies) &;
  Request cookies(
    const std::unordered_map<std::string, std::string>& cookies) &&;
  /// Follow redirects or not. Default: follow
  Request& follow_redirects(bool follow = true) &;
  Request follow_redirects(bool follow = true) &&;
  /// Set timeout in ms for request
  Request& timeout(long timeout_ms) &;
  Request timeout(long timeout_ms) &&;
  Request& timeout(const std::chrono::milliseconds& timeout_ms) &;
  Request timeout(const std::chrono::milliseconds& timeout_ms) &&;
  /// Verify host and peer or not. Default: verify
  Request& verify(bool verify = true) &;
  Request verify(bool verify = true) &&;
  /// Set file holding one or more certificates to verify the peer with
  Request& ca_info(const std::string& file_path) &;
  Request ca_info(const std::string& file_path) &&;
  /// Set CA
  Request& ca(const Certificate& cert) &;
  Request ca(const Certificate& cert) &&;
  /// Set CRL-file
  Request& crl_file(const std::string& file_path) &;
  Request crl_file(const std::string& file_path) &&;
  /// Set private client key and certificate for request.
  ///
  /// @warning Do not use this function on MacOS as it may cause Segmentation
  /// Fault on that platform.
  Request& client_key_cert(
    const PrivateKey& pkey,
    const Certificate& cert) &;
  Request client_key_cert(
    const PrivateKey& pkey,
    const Certificate& cert) &&;
  /// Set HTTP version
  Request& http_version(const HttpVersion version) &;
  Request http_version(const HttpVersion version) &&;

  /// Specify number of retries on incorrect status, if on_fails is True
  /// retry on network error too. Retries = 3 means that maximum 3 request
  /// will be performed.
  ///
  /// Retries use exponential backoff - an exponentially increasing delay
  /// is added before each retry of this request.
  Request& retry(
    const short retries = 3,
    const bool on_fails = true) &;
  Request retry(
    const short retries = 3,
    const bool on_fails = true) &&;

  /// Set unix domain socket as connection endpoint and provide path to it
  /// When enabled, request will connect to the Unix domain socket instead
  /// of establishing a TCP connection to a host.
  Request& unix_socket_path(const std::string& path) &;
  Request unix_socket_path(const std::string& path) &&;

  /// Set CURL_IPRESOLVE_V4 for ipv4 resolving
  Request& use_ipv4() &;
  Request use_ipv4() &&;
  /// Set CURL_IPRESOLVE_V6 for ipv6 resolving
  Request& use_ipv6() &;
  Request use_ipv6() &&;

  /// Set CURLOPT_CONNECT_TO option
  Request& connect_to(const ConnectTo& connect_to) &;
  Request connect_to(const ConnectTo& connect_to) &&;

  /// Override log URL. Usefull for "there's a secret in the query".
  /// @warning The query might be logged by other intermediate HTTP agents
  ///          (nginx, L7 balancer, etc.).
  Request& set_logged_url(const std::string& url) &;
  Request set_logged_url(const std::string& url) &&;

  /// Set destination name in metric "httpclient.destinations.<name>".
  /// If not set, defaults to HTTP path.  Should be called for all requests
  /// with parameters in HTTP path.
  Request& set_destination_metric_name(const std::string& destination) &;
  Request set_destination_metric_name(const std::string& destination) &&;

  Request& set_allowed_urls_extra(const std::vector<std::string>& urls) &;

  /// Disable auto-decoding of received replies.
  /// Useful to proxy replies 'as is'.
  Request& disable_reply_decoding() &;
  Request disable_reply_decoding() &&;

  void set_cancellation_policy(const CancellationPolicy cp);

  std::shared_ptr<Response> perform(
    const SourceLocation& location = SourceLocation::Current());

private:
  Request(
    const Impl& impl,
    TaskProcessor& task_processor);

  Request(
    Impl&& impl,
    TaskProcessor& task_processor);

private:
  friend class Client;

  TaskProcessor& task_processor_;

  ImplPtr impl_;
};

} // namespace UServerUtils::Http::Client

#endif //USERVER_HTTP_CLIENT_REQUEST_HPP