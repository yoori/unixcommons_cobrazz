#include "../Overload/Server/TestInt.hpp"

#include "Init.hpp"

void
Client::work() /*throw (eh::Exception)*/
{
  CORBATest::OctetSeq param;
  if (opt_secure_url.installed())
  {
    (*opt_secure_url)->test(param);
  }
  if (opt_url.installed())
  {
    (*opt_url)->test(param);
  }
}

int
main(int argc, char** argv)
{
  Usage<Client, Server> usage;
  return usage.use(argc, argv);
}
