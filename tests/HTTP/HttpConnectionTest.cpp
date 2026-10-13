#include <iostream>
#include <stdexcept>
#include <string>

#include <HTTP/HttpConnection.hpp>

namespace
{
  void require(bool condition, const char* message)
  {
    if (!condition)
    {
      throw std::runtime_error(message);
    }
  }

  class Callback : public HTTP::ResponseCallback, public ReferenceCounting::AtomicImpl
  {
  public:
    void on_response(const HTTP::ResponseInformation& response) noexcept override;
    void on_error(const String::SubString& error, const HTTP::RequestInformation&)
      noexcept override;

    unsigned int calls = 0;
    int status = 0;
    std::string body;
    std::string error;
  };

  void Callback::on_response(const HTTP::ResponseInformation& response) noexcept
  {
    ++calls;
    status = response.response_code();
    body = response.body().str();
  }

  void Callback::on_error(const String::SubString& description, const HTTP::RequestInformation&)
    noexcept
  {
    ++calls;
    error = description.str();
  }

  void test(const std::string& mode, const char* url, const char* proxy)
  {
    const Generics::Time timeout(1);
    const Generics::Time short_timeout(0, 100000);
    const Generics::Time zero;
    if (mode.find("wrapper") == 0)
    {
      ReferenceCounting::SmartPtr<Callback> callback = new Callback;
      HTTP::HttpInterface_var client = HTTP::CreateSyncHttp(&timeout, &timeout, &short_timeout);
      client->add_get_request(url, callback.in());
      require(callback->calls == 1, "callback count");
      if (mode == "wrapper-timeout")
      {
        require(!callback->error.empty(), "missing wrapper error");
      }
      else
      {
        require(callback->error.empty(), "unexpected wrapper error");
        require(callback->status == (mode == "wrapper-status" ? 404 : 204), "wrapper status");
        require(mode == "wrapper-status" || callback->body.empty(), "empty wrapper body");
      }

      std::cout << "WRAPPER\n";
      return;
    }

    HTTP::HTTP_Connection connection{HTTP::HTTPAddress(String::SubString(url)), proxy};
    const int count = mode == "reconnect" ? 2 : 1;
    for (int iteration = 0; iteration < count; ++iteration)
    {
      try
      {
        connection.connect(mode == "connect-timeout" ? &zero : &timeout);
        connection.set_linger(false, 0);
        HTTP::HTTP_Connection::HttpBody body;
        HTTP::HeaderList headers;
        HTTP::ParamList params;
        auto method = HTTP::HTTP_Connection::HM_Get;
        if (mode == "post" || mode == "no-response" || mode == "send-timeout")
        {
          method = HTTP::HTTP_Connection::HM_Post;
          body = mode == "send-timeout" ?
            std::string(32 * 1024 * 1024, 'x') : std::string("a\0b", 3);
        }

        if (mode == "form")
        {
          method = HTTP::HTTP_Connection::HM_Post;
        }

        if (mode == "get" || mode == "post" || mode == "form" || mode == "proxy")
        {
          params.emplace_back("x y", "a+b");
        }

        headers.emplace_back("X-Repeat", "one");
        headers.emplace_back("X-Repeat", "two");
        if (mode == "host")
        {
          headers.emplace_back("hOsT", "override.test");
        }

        unsigned int sent = 0;
        unsigned int received = 0;
        Generics::Time latency;
        const auto status = connection.process_request(method, params, headers, body,
          mode != "no-response", &short_timeout, &short_timeout, &sent, &received, &latency);
        require(mode.find("timeout") == std::string::npos, "timeout was not enforced");
        require(mode != "invalid" && mode != "truncated", "invalid response accepted");
        require(mode != "status", "error status accepted");
        if (mode == "no-response")
        {
          require(status == 0 && body == std::string("a\0b", 3), "no-response result");
        }
        else
        {
          require(status == (mode == "empty" ? 204U : mode == "redirect" ? 302U : 200U),
            "unexpected status");
          const auto expected = mode == "large-body" ? std::string(9 * 1024 * 1024, 'x') :
            mode == "empty" ? std::string() : std::string("reply\0data", 10);
          require(body == expected, "response body mismatch");
          require(latency >= Generics::Time::ZERO, "negative latency");
        }

        std::cout << sent << ' ' << received << '\n';
      }
      catch (const HTTP::HTTP_Connection::Timeout&)
      {
        require(mode.find("timeout") != std::string::npos, "unexpected timeout");
        std::cout << "TIMEOUT\n";
      }
      catch (const HTTP::HTTP_Connection::StatusException& error)
      {
        require(mode == "status" && error.status == 404, "wrong status exception");
        std::cout << "STATUS\n";
      }
      catch (const HTTP::HTTP_Connection::Exception&)
      {
        require(mode == "invalid" || mode == "truncated" || mode == "refused",
          "unexpected connection exception");
        std::cout << "ERROR\n";
      }
    }
  }
}

int main(int argc, char** argv)
{
  try
  {
    require(argc == 3 || argc == 4, "usage: HttpConnectionTest MODE URL [PROXY]");
    test(argv[1], argv[2], argc == 4 ? argv[3] : nullptr);
    return 0;
  }
  catch (const std::exception& error)
  {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
