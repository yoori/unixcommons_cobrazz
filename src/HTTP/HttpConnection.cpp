#include <chrono>
#include <cstdint>
#include <limits>
#include <utility>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <String/StringManip.hpp>
#include <HTTP/HttpConnection.hpp>

namespace HTTP
{
  namespace
  {
    namespace asio = boost::asio;
    namespace beast = boost::beast;
    namespace http = beast::http;
    using ErrorCode = boost::system::error_code;
    using Clock = std::chrono::steady_clock;

    std::chrono::microseconds timeout_duration(const Generics::Time& timeout)
    {
      if (timeout < Generics::Time::ZERO)
      {
        throw HTTP_Connection::InvalidArgs("Negative HTTP timeout");
      }

      return std::chrono::microseconds(timeout.microseconds());
    }

    void check_error(const ErrorCode& error)
    {
      if (error == beast::error::timeout || error == asio::error::timed_out)
      {
        throw HTTP_Connection::Timeout(error.message());
      }

      if (error)
      {
        throw HTTP_Connection::Exception(error.message());
      }
    }

    std::string resolver_host(std::string host)
    {
      if (host.size() > 1 && host.front() == '[' && host.back() == ']')
      {
        host = host.substr(1, host.size() - 2);
      }

      return host;
    }
  }

  struct HTTP_Connection::Impl
  {
    struct InputStream
    {
      using executor_type = beast::tcp_stream::executor_type;

      executor_type get_executor() noexcept;

      template<typename Buffers, typename Handler>
      void async_read_some(const Buffers& buffers, Handler&& handler);

      Impl& owner;
      const Generics::Time* timeout;
    };

    Impl();
    void close() noexcept;
    void expires(const Generics::Time* timeout);

    template<typename Start>
    std::size_t run_io(Start&& start);

    asio::io_context context;
    beast::tcp_stream stream;
  };

  HTTP_Connection::Impl::Impl() : stream(context)
  {
  }

  HTTP_Connection::Impl::InputStream::executor_type
  HTTP_Connection::Impl::InputStream::get_executor() noexcept
  {
    return owner.stream.get_executor();
  }

  template<typename Buffers, typename Handler>
  void
  HTTP_Connection::Impl::InputStream::async_read_some(const Buffers& buffers, Handler&& handler)
  {
    // Match SocketStream's timeout for each socket read, not the entire response.
    owner.expires(timeout);
    owner.stream.async_read_some(buffers, std::forward<Handler>(handler));
  }

  void HTTP_Connection::Impl::close() noexcept
  {
    ErrorCode ignored;
    stream.socket().close(ignored);
  }

  void HTTP_Connection::Impl::expires(const Generics::Time* timeout)
  {
    if (timeout)
    {
      stream.expires_after(timeout_duration(*timeout));
    }
    else
    {
      stream.expires_never();
    }
  }

  template<typename Start>
  std::size_t HTTP_Connection::Impl::run_io(Start&& start)
  {
    ErrorCode error;
    std::size_t transferred = 0;
    context.restart();
    start([&error, &transferred](ErrorCode result, std::size_t size)
    {
      error = result;
      transferred = size;
    });
    context.run();
    check_error(error);
    return transferred;
  }

  HTTP_Connection::HTTP_Connection(const HTTPAddress& url, const char* proxy)
    : impl_(std::make_unique<Impl>()), url_(url), proxy_port_(0)
  {
    if (proxy && *proxy)
    {
      HTTPAddress address("http://" + std::string(proxy));
      proxy_host_ = address.host().str();
      proxy_port_ = address.port().empty() ? 3128 : address.port_number();
    }
  }

  HTTP_Connection::~HTTP_Connection() noexcept = default;

  void HTTP_Connection::connect(const Generics::Time* connect_timeout)
  {
    impl_->close();
    impl_->stream.expires_never();
    impl_->context.restart();
    asio::ip::tcp::resolver resolver(impl_->context);
    asio::steady_timer timer(impl_->context);
    ErrorCode error;
    bool timed_out = false;

    if (connect_timeout)
    {
      timer.expires_after(timeout_duration(*connect_timeout));
      timer.async_wait([&](ErrorCode result)
      {
        if (!result)
        {
          timed_out = true;
          resolver.cancel();
          impl_->close();
        }
      });
    }

    const auto host = resolver_host(proxy_host_.empty() ? url_.host().str() : proxy_host_);
    const auto port = std::to_string(proxy_host_.empty() ? url_.port_number() : proxy_port_);
    resolver.async_resolve(host, port,
      [&](ErrorCode result, asio::ip::tcp::resolver::results_type endpoints)
      {
        error = result;
        if (result || timed_out)
        {
          timer.cancel();
          return;
        }

        impl_->stream.async_connect(endpoints, [&](ErrorCode result, const asio::ip::tcp::endpoint&)
        {
          error = result;
          timer.cancel();
        });
      });
    impl_->context.run();
    if (timed_out)
    {
      throw Timeout("HTTP connection timed out");
    }

    check_error(error);
  }

  void HTTP_Connection::set_linger(bool enabled, int seconds)
  {
    ErrorCode error;
    impl_->stream.socket().set_option(asio::socket_base::linger(enabled, seconds), error);
    check_error(error);
  }

  unsigned int HTTP_Connection::process_request(
    HTTP_Method method, const ParamList& params, HeaderList& headers, HttpBody& body,
    bool need_response, const Generics::Time* send_timeout, const Generics::Time* recv_timeout,
    unsigned int* bytes_sent, unsigned int* bytes_rcvd, Generics::Time* response_latency)
  {
    try
    {
      if (method != HM_Get && method != HM_Post)
      {
        throw InvalidArgs("Unknown HTTP method");
      }

      std::string target = proxy_host_.empty() ? url_.path().str() : url_.url();
      std::string query = proxy_host_.empty() ? url_.query().str() : std::string();
      for (const auto& param : params)
      {
        std::string name;
        std::string value;
        String::StringManip::mime_url_encode(param.name, name);
        String::StringManip::mime_url_encode(param.value, value);
        if (!query.empty())
        {
          query += '&';
        }

        query += name + '=' + value;
      }

      if (!query.empty())
      {
        if (method == HM_Get || body)
        {
          target += !proxy_host_.empty() && !url_.query().empty() ? '&' : '?';
          target += query;
        }
        else
        {
          body = std::move(query);
        }
      }

      http::request<http::string_body> request(
        method == HM_Get ? http::verb::get : http::verb::post, target, 10);
      for (const auto& header : headers)
      {
        request.insert(header.name, header.value);
      }

      if (request.find(http::field::host) == request.end())
      {
        std::string host = url_.host().str();
        if (url_.port_number() != 80)
        {
          host += ':' + std::to_string(url_.port_number());
        }

        request.set(http::field::host, host);
      }

      if (body)
      {
        request.body() = *body;
      }

      request.content_length(request.body().size());
      http::request_serializer<http::string_body> serializer(request);
      impl_->expires(send_timeout);
      auto sent = impl_->run_io([&](auto handler)
      {
        http::async_write_header(impl_->stream, serializer, std::move(handler));
      });
      impl_->expires(send_timeout);
      sent += impl_->run_io([&](auto handler)
      {
        http::async_write(impl_->stream, serializer, std::move(handler));
      });
      if (bytes_sent)
      {
        *bytes_sent = sent;
      }

      if (!need_response)
      {
        impl_->close();
        return 0;
      }

      headers.clear();
      body.reset();
      beast::flat_buffer buffer;
      http::response_parser<http::string_body> parser;
      parser.body_limit(std::numeric_limits<std::uint64_t>::max());
      parser.header_limit(std::numeric_limits<std::uint32_t>::max());
      Impl::InputStream input{*impl_, recv_timeout};
      const auto started = Clock::now();
      const auto first = impl_->run_io([&](auto handler)
      {
        input.async_read_some(buffer.prepare(8192), std::move(handler));
      });
      buffer.commit(first);
      if (response_latency)
      {
        const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
          Clock::now() - started).count();
        *response_latency = Generics::Time(elapsed / 1000000, elapsed % 1000000);
      }

      auto received = impl_->run_io([&](auto handler)
      {
        http::async_read_header(input, buffer, parser, std::move(handler));
      });
      const auto status = parser.get().result_int();
      if (status < 200 || status >= 400)
      {
        throw StatusException("HTTP status " + std::to_string(status) + ": " +
          std::string(parser.get().reason()), status);
      }

      received += impl_->run_io([&](auto handler)
      {
        http::async_read(input, buffer, parser, std::move(handler));
      });
      for (const auto& header : parser.get().base())
      {
        headers.emplace_back(std::string(header.name_string()), std::string(header.value()));
      }

      body = std::move(parser.get().body());
      if (bytes_rcvd)
      {
        *bytes_rcvd = received + buffer.size();
      }

      impl_->close();
      return status;
    }
    catch (...)
    {
      impl_->close();
      throw;
    }
  }
}
