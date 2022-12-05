#include <String/StringManip.hpp>

#include <CORBAConfigParser/ParameterConfig.hpp>

namespace
{
  struct Token
  {
    const char* name;
    bool empty;
  };

  const Token TOKENS[] =
  {
    { "key file", false },
    { "password", true },
    { "certificate", false },
    { "certificate authority", false },
  };
  const size_t NUMBER_OF_TOKENS = sizeof(TOKENS) / sizeof(*TOKENS);
}

namespace CORBAConfigParser
{
  void
  parse_secure_params_arg(const String::SubString& str,
    CORBACommons::SecureConnectionConfig& config)
    /*throw (eh::Exception, Generics::AppUtils::InvalidParam)*/
  {
    try
    {
      String::StringManip::Splitter<String::AsciiStringManip::SepColon,
        true> tokenizer(str);
      String::SubString token;
      std::string values[NUMBER_OF_TOKENS];
      for (size_t i = 0; i < NUMBER_OF_TOKENS; i++)
      {
        if (!tokenizer.get_token(token) ||
          (token.empty() && !TOKENS[i].empty))
        {
          Stream::Error ostr;
          ostr << FNS << "Not defined " << TOKENS[i].name;
          throw Generics::AppUtils::InvalidParam(ostr);
        }
        token.assign_to(values[i]);
      }

      config.parse(values[0].c_str(), values[1].c_str(), values[2].c_str(),
        values[3].c_str());
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FNS << "Error parsing secure params '" << str <<
        "': " << ex.what();
      throw Generics::AppUtils::InvalidParam(ostr);
    }
  }

  void
  SecureParamsOption::set(const char*, const char* strval)
    /*throw (eh::Exception, Generics::AppUtils::InvalidParam)*/
  {
    try
    {
      CORBACommons::SecureConnectionConfig val;
      parse_secure_params_arg(String::SubString(strval), val);
      set_value(val);
    }
    catch (const CORBACommons::SecureConnectionConfig::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FNS <<
        "Can't initialize secure connection. Caught Exception: " <<
        ex.what();
      throw Generics::AppUtils::InvalidParam(ostr);
    }
  }
}
