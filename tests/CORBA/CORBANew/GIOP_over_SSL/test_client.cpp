#include <iostream>
#include <vector>
#include <signal.h>
#include "echo.hpp"
#ifdef ORB_OMNI
#include <omniORB4/sslContext.h>
#endif

static void
hello(Echo_ptr e)
{
  CORBA::String_var src = (const char*) "Hello!";
  CORBA::String_var dest = e->echoString(src);

  std::cout << "I said, \"" << src << "\"." << std::endl
       << "The Echo object replied, \"" << dest <<"\"." << std::endl;
}

int
main(int argc, char** argv)
{
#ifdef ORB_OMNI
  char CERTIFICATE[] = "root.pem";
  char KEY[] = "client.pem";
  char PASSWORD[] = "password";
#endif
#ifdef ORB_MICO
  char CERTIFICATE[] = "c_cert.pem";
  char KEY[] = "c_key.pem";
#endif

#ifdef ORB_OMNI
  sslContext::certificate_authority_file = CERTIFICATE;
  sslContext::key_file = KEY;
  sslContext::key_file_password = PASSWORD;

  CORBA::ORB_var orb = CORBA::ORB_init(argc, argv, ORB_NAME);
#else
  std::vector<char*> args;
  args.resize(argc);
  std::copy(argv, argv + argc, args.begin());
#ifdef ORB_MICO
  args.push_back("-ORBSSLcert");
  args.push_back(CERTIFICATE);
  args.push_back("-ORBSSLkey");
  args.push_back(KEY);
#else
  args.push_back("-ORBSvcConf");
  args.push_back("client.conf");
#endif
  args.push_back(0);
  int argsc = args.size() - 1;
  CORBA::ORB_var orb = CORBA::ORB_init(argsc, &args[0], ORB_NAME);
#endif

  if (argc != 2)
  {
    return 1;
  }

  CORBA::Object_var obj = orb->string_to_object(argv[1]);
  Echo_var echoref = Echo::_narrow(obj);
  if (CORBA::is_nil(echoref))
  {
    std::cerr << "Can't narrow reference to type Echo (or it was nil)." << std::endl;
    return 1;
  }

  for (CORBA::ULong count = 0; count < 10; count++)
  {
    try
    {
      hello(echoref);
    }
    catch (...)
    {
      std::cout << "Exception caught" << std::endl;
    }
  }

  orb->destroy();

  kill(getppid(), SIGINT);

  return 0;
}
