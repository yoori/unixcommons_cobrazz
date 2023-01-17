#include <iostream>
#include <sstream>

#include <Generics/AppUtils.hpp>

#include <Logger/StreamLogger.hpp>

#include <CORBACommons/CorbaAdapters.hpp>
#include <CORBAConfigParser/ParameterConfig.hpp>

//#include "../Server/TestCrash.hpp"
#include "tests/CrashCall/TestCrash.hpp"

int
main(int argc, char** argv)
{
  try
  {
    Logging::FLogger_var logger(
      new Logging::OStream::Logger(Logging::OStream::Config(std::cout)));

    CORBACommons::CorbaClientAdapter_var corba_client_adapter(
      new CORBACommons::CorbaClientAdapter(logger));

    CORBAConfigParser::CorbaRefOption<CORBATest::TestCrash> opt_url(
      corba_client_adapter.in());
    CORBAConfigParser::CorbaRefOption<CORBATest::TestCrash> opt_secure_url(
      corba_client_adapter.in(),
      "server.key:adserver:server.der;ce.der");

    Generics::AppUtils::Args args;

    args.add(
      Generics::AppUtils::equal_name("url") ||
      Generics::AppUtils::short_name("u"),
      opt_url);
    args.add(
      Generics::AppUtils::equal_name("secure-url") ||
      Generics::AppUtils::short_name("su"),
      opt_secure_url);

    args.parse(argc - 1, argv + 1);

    if (!opt_url.installed() && !opt_secure_url.installed())
    {
      std::cerr << "Neither secure nor insecure url is supplied" << std::endl;
      return -1;
    }

    {
      CORBATest::TestCrash_var test_int;
      if (opt_url.installed())
      {
        std::cout << "Testing insecure connection" << std::endl;
        test_int = *opt_url;
      }
      else
      {
        std::cout << "Testing secure connection" << std::endl;
        test_int = *opt_secure_url;
      }

      try
      {
        test_int->crash();
        std::cerr << "Haven't got expected exception" << std::endl;
        return -1;
      }
      catch (const CORBA::Exception& ex)
      {
        std::cout << "Got expected exception: " << ex << std::endl;
      }
    }

    return 0;
  }
  catch (const CORBA::Exception& e)
  {
    std::cerr << "CORBA::Exception:" << e;
  }
  catch (const eh::Exception& e)
  {
    std::cerr << "eh::Exception:" << e.what() << std::endl;
  }
  catch (...)
  {
    std::cerr << "Unknown exception" << std::endl;
  }

  return -1;
}
