#ifndef USERVER_HTTP_CLIENT_CLIENT_HPP
#define USERVER_HTTP_CLIENT_CLIENT_HPP

// USERVER
#include <userver/clients/http/client.hpp>

// THIS
#include <UServerUtils/Http/Client/Config.hpp>
#include <UServerUtils/Http/Client/Request.hpp>

namespace UServerUtils::Http::Client
{

class Client final
{
private:
  using Impl = userver::clients::http::Client;
  using ImplPtr = std::shared_ptr<Impl>;

public:
  explicit Client(
    const ClientConfig& config,
    userver::engine::TaskProcessor& task_processor);

  ~Client();

  /* *
   * Returns a HTTP request builder type with preset values of
   * User-Agent, Proxy and some of the Testsuite suff (if any).
   * This method is thread-safe despite being non-const.
   * */
  Request create_request();

  /* *
   * Providing CreateNonSignedRequest() function for the clients::Http alias.
   * This method is thread-safe despite being non-const.
   * */
  Request create_not_signed_request();

  /* *
   * Sets User-Agent headers for all the requests or removes that header.
   * By default User-Agent is set by components::HttpClient to the
   * userver identity string.
   * */
  void reset_user_agent(
    std::optional<std::string> user_agent = std::nullopt);

  /* *
   * Returns the current proxy that is automatically used for each
   * @warning The value may become immediately obsole as the proxy could be
   * concurrently changed from runtime config.
   * */
  std::string get_proxy() const;

  /* *
   * Sets the DNS resolver to use.
   * If given nullptr, the default resolver will be used
   * (most likely getaddrinfo).
   * */
  void set_dns_resolver(userver::clients::dns::Resolver* resolver);

private:
  ImplPtr impl_;

  userver::engine::TaskProcessor& task_processor_;
};

using ClientPtr = std::shared_ptr<Client>;

} // namespace UServerUtils::Http::Client

#endif //USERVER_HTTP_CLIENT_CLIENT_HPP