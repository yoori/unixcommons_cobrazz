#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <HTTP/UrlAddress.hpp>

namespace
{
  void require(bool condition, const char* message)
  {
    if (!condition)
    {
      throw std::runtime_error(message);
    }
  }

  class BrowserProbe : public HTTP::BrowserAddress
  {
  public:
    explicit BrowserProbe(bool force_rebuild = false) : force_rebuild_(force_rebuild) {}

    bool host_changed = false;
    const char* buffer_before_checks = nullptr;

    std::string reference_view(unsigned long flags) const
    {
      std::string result;
      if (flags & VW_PROTOCOL)
      {
        if (parts_.has_scheme)
        {
          result += secure() ? "https:" : "http:";
        }

        result += "//";
      }

      if (flags & VW_HOSTNAME)
      {
        if ((flags & VW_HOSTNAME_WWW) == VW_HOSTNAME_WWW &&
          !String::AsciiStringManip::Caseless("www.").start(host()))
        {
          result += "www.";
        }

        host().append_to(result);
      }

      if ((flags & VW_PORT) || (!is_default_port() && (flags & VW_NDEF_PORT)))
      {
        result += ':' + std::to_string(port_number());
      }

      if (flags & VW_PATH)
      {
        auto length = path().size();
        if ((flags & VW_STRIP_PATH) == VW_STRIP_PATH && length && path()[length - 1] == '/')
        {
          --length;
        }

        result.append(path().data(), length);
      }

      if ((flags & VW_QUERY) && parts_.has_query)
      {
        result += '?';
        query().append_to(result);
      }

      if ((flags & VW_FRAGMENT) && parts_.has_fragment)
      {
        result += '#';
        fragment().append_to(result);
      }

      return result;
    }

  protected:
    bool additional_checks_() override
    {
      buffer_before_checks = url().data();
      host_changed = HTTP::BrowserAddress::additional_checks_();
      return force_rebuild_ || host_changed;
    }

  private:
    bool force_rebuild_;
  };
}

int main()
{
  try
  {
    const std::vector<std::string> inputs = {
      "http://example.com/path", "example.com", "//example.com/path", "http://example.com",
      "https://EXAMPLE.COM:443/path?q=x#f", "http://www.example.com/", "http://example.com/?#",
      "http://user:pass@example.com:8080/a b?q=a b#c d", "http://пример.рф/путь",
      "http://xn--e1afmkfd.xn--p1ai/", "http://example.com/?q=" + std::string(10000, 'x')
    };
    for (const auto& input : inputs)
    {
      BrowserProbe current;
      BrowserProbe previous(true);
      current.url(input);
      previous.url(input);
      require(current.url() == previous.url(), "URL differs from forced reconstruction");
      require(current.host() == previous.host(), "host differs from forced reconstruction");
      require(current.path() == previous.path(), "path differs from forced reconstruction");
      for (unsigned long flags = 0; flags < 512; ++flags)
      {
        std::string result;
        current.get_view(flags, result);
        require(result == previous.reference_view(flags), "get_view output changed");
      }
    }

    BrowserProbe unchanged;
    unchanged.url(std::string_view("http://example.com/long-enough-path?q=value"));
    require(!unchanged.host_changed, "unchanged host requested reconstruction");
    require(unchanged.url().data() == unchanged.buffer_before_checks,
      "unchanged URL buffer was replaced");
    require(unchanged.host().data() == unchanged.url().data() + 7,
      "host view does not belong to URL buffer");

    std::string result;
    const auto capacity = result.capacity();
    unchanged.get_view(HTTP::HTTPAddress::VW_HOSTNAME, result);
    require(result == "example.com" && result.capacity() == capacity,
      "short host view unnecessarily allocated a buffer");
    std::cout << "TestHTTPUrlOptimization: PASS\n";
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
