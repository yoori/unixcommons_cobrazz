#pragma once

#include <memory>
#include <optional>
#include <string>

#include <Generics/Time.hpp>
#include <HTTP/UrlAddress.hpp>
#include <HTTP/Http.hpp>

namespace HTTP
{
  class HTTP_Connection
  {
  public:
    enum HTTP_Method
    {
      HM_Post,
      HM_Get
    };

    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    DECLARE_EXCEPTION(InvalidArgs, Exception);
    DECLARE_EXCEPTION(Timeout, Exception);

    class StatusException : public Exception
    {
    public:
      unsigned int status;

      StatusException(const char* description, unsigned int status) noexcept;
      StatusException(const std::string& description, unsigned int status) noexcept;
      StatusException(const Stream::Error& description, unsigned int status) noexcept;
    };

    // An absent POST body allows parameters to be sent as form data.
    using HttpBody = std::optional<std::string>;

    explicit HTTP_Connection(const HTTPAddress& url, const char* proxy = nullptr);
    virtual ~HTTP_Connection() noexcept;

    void connect(const Generics::Time* connect_timeout = nullptr);
    void set_linger(bool enabled, int seconds);

    unsigned int process_request(
      HTTP_Method method, const ParamList& params, HeaderList& headers, HttpBody& body,
      bool need_response = true, const Generics::Time* send_timeout = nullptr,
      const Generics::Time* recv_timeout = nullptr, unsigned int* bytes_sent = nullptr,
      unsigned int* bytes_rcvd = nullptr, Generics::Time* response_latency = nullptr);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    HTTPAddress url_;
    std::string proxy_host_;
    unsigned short proxy_port_;
  };

  inline
  HTTP_Connection::StatusException::StatusException(const char* description, unsigned int status)
    noexcept
    : Exception(description), status(status)
  {
  }

  inline HTTP_Connection::StatusException::StatusException(
    const std::string& description, unsigned int status) noexcept
    : Exception(description), status(status)
  {
  }

  inline HTTP_Connection::StatusException::StatusException(
    const Stream::Error& description, unsigned int status) noexcept
    : Exception(description), status(status)
  {
  }
}
