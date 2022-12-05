#include <iostream>
#include <assert.h>

#include <eh/Exception.hpp>

#include <String/StringManip.hpp>
#include <String/UTF8Handler.hpp>

#include <Stream/MMapStream.hpp>

#include <HTTP/UrlAddress.hpp>


class IDNANorm
{
public:
  int
  run() /*throw (eh::Exception)*/;

private:
  static const char* data_[][2];
};

const char* IDNANorm::data_[][2] =
{
  {
    "http://a.com",
    "a.com"
  },
  {
    "a.com",
    "a.com"
  },
  {
    "a.com/path",
    "a.com/path"
  },
  {
    "a.com?query",
    "a.com?query"
  },
  {
    "a.com/path?query",
    "a.com/path?query"
  },
  {
    "http://a.com/path?query#fragment",
    "a.com/path?query"
  },
  {
    "A.COM",
    "a.com"
  },
  {
    "http://пример.испытание/пример.испытание?пример.испытание",
    "пример.испытание/пример.испытание?пример.испытание"
  },
  {
    "http://ПрИмЕр.ИсПыТаНиЕ/ПрИмЕр.иСпЫтАнИе?пРиМеР.иСпЫтАнИе",
    "пример.испытание/ПрИмЕр.иСпЫтАнИе?пРиМеР.иСпЫтАнИе"
  },
  {
    "xn--e1afmkfd.xn--80akhbyknj4f/%33%5%20?%41%",
    "пример.испытание/3%5 ?A%"
  },
  {
    "Xn--E1AfMkFd.xN--80aKhByKnJ4f/%%33%5%20?%%41%",
    "пример.испытание/%3%5 ?%A%"
  },
  {
    "http://xn--20202034202020-opa11bsake8a5ft1lnxhk4aee49mk41tvma."
      "dldaylight.info/register.php",
    "xn--20202034202020-opa11bsake8a5ft1lnxhk4aee49mk41tvma."
      "dldaylight.info/register.php"
  },
  {
    "http://xn--20202034202020a-oqa63btale22e0c79hh9it5akeq066d."
      "dldaylight.info/register.php",
    "ɑȟ2020ɋ2ᤌ03à3⁄44ä2023⁄4ļ01⁄421⁄20£a.dldaylight.info/register.php"
  },
  {
    "http://xn--2020203344202340142120a-wya32mcd36id2julb8ev923ezjybhaed."
      "dldaylight.info/register.php",
    "ɑȟ2020ɋ2ᤌ03à3⁄44ä2023⁄4ļ01⁄421⁄20£a.dldaylight.info/register.php"
  },
  {
    0,
    0
  }
};

int
IDNANorm::run() /*throw (eh::Exception)*/
{
  for (size_t i = 0; data_[i][0]; i++)
  {
    try
    {
      std::string res(HTTP::keywords_from_http_address(
        String::SubString(data_[i][0])));
      if (data_[i][1])
      {
        if (data_[i][1] != res)
        {
          std::cerr << "Failed to convert '" << data_[i][0] << "' got '" <<
            res << "' instead of '" << data_[i][1] << "'" << std::endl;
        }
      }
      else
      {
        std::cerr << "Failed to convert '" << data_[i][0] << "' got '" <<
          res << "' instead of exception" << std::endl;
      }
    }
    catch (const eh::Exception& ex)
    {
      if (data_[i][1])
      {
        std::cerr << "Failed to convert '" << data_[i][0] <<
          "' got exception " << ex.what() << " instead of '" <<
          data_[i][1] << "'" << std::endl;
      }
    }
  }
  return 0;
}

int
main()
{
  try
  {
    IDNANorm appnorm;
    return appnorm.run();
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << " eh::Exception caught. Description:" << ex.what() <<
      std::endl;
  }

  return 1;
}
