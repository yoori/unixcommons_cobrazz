#include <iostream>
#include <vector>
#include <signal.h>
#include "echo.hpp"

int
main(int argc, char** argv)
{
  if (!fork())
  {
    sleep(2);
    system("netstat -an | grep :102[56]");
    kill(getppid(), SIGINT);
    exit(0);
  }
#ifdef ORB_OMNI
  char ADDR1[] = "giop:tcp:localhost:1025";
  char ADDR2[] = "giop:tcp:localhost:1026";
#else
  char ADDR1[] = "inet:localhost:1025";
  char ADDR2[] = "inet:localhost:1026";
#endif

#ifdef ORB_OMNI
  const char* options[][2] =
    {
      { "endPoint", ADDR1 },
      { "endPoint", ADDR2 },
      { 0, 0 }
    };
  CORBA::ORB_var orb = CORBA::ORB_init(argc, argv, ORB_NAME, options);
#else
  std::vector<char*> args;
  args.resize(argc);
  std::copy(argv, argv + argc, args.begin());
  args.push_back("-ORBIIOPAddr");
  args.push_back(ADDR1);
  args.push_back("-ORBIIOPAddr");
  args.push_back(ADDR2);
  args.push_back(0);
  int argsc = args.size() - 1;
  CORBA::ORB_var orb = CORBA::ORB_init(argsc, &args[0], ORB_NAME);
#endif

  CORBA::Object_var obj = orb->resolve_initial_references("RootPOA");
  PortableServer::POA_var poa = PortableServer::POA::_narrow(obj);
  PortableServer::POAManager_var pman = poa->the_POAManager();
  pman->activate();

  orb->run();

  return 0;
}
