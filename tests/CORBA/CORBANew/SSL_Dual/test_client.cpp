#include <iostream>
#include <vector>
#include <signal.h>
#include "echo.hpp"

class Echo_i : public POA_Echo
{
public:
  virtual char*
  echoString(const char* message, const char* ior) throw ();
};

char*
Echo_i::echoString(const char* message, const char*) throw ()
{
  std::cerr << "Client!\n";
  return CORBA::string_dup(message);
}

static void
hello(Echo_ptr e, const char* ior)
{
  CORBA::String_var src = (const char*) "Hello!";
  CORBA::String_var dest = e->echoString(src, ior);

  std::cout << "I said, \"" << src << "\"." << std::endl
       << "The Echo object replied, \"" << dest <<"\"." << std::endl;
}

void*
thread_proc(void* arg)
{
  (*static_cast<CORBA::ORB_var*>(arg))->run();
  return 0;
}

int
main(int argc, char** argv)
{
  //char ADDR[] = "iiop://0.0.0.0:1028/ssl_port=1027";
  char ADDR[] = "iiop://localhost:1028";

  std::vector<char*> args;

  args.resize(argc);
  std::copy(argv, argv + argc, args.begin());
  args.push_back("-ORBSvcConf");
  args.push_back("server_2.conf");
  args.push_back("-ORBEndpoint");
  args.push_back(ADDR);
  args.push_back(0);
  int argsc = args.size() - 1;
  CORBA::ORB_var orb_s = CORBA::ORB_init(argsc, &args[0], ORB_NAME "_2");

  args.resize(argc);
  std::copy(argv, argv + argc, args.begin());
  args.push_back("-ORBSvcConf");
  args.push_back("client_1.conf");
  args.push_back(0);
  argsc = args.size() - 1;
  CORBA::ORB_var orb_c = CORBA::ORB_init(argsc, &args[0], ORB_NAME "_1");

  CORBA::Object_var obj = orb_s->resolve_initial_references("RootPOA");
  PortableServer::POA_var poa = PortableServer::POA::_narrow(obj);

  PortableServer::POAManager_var pman = poa->the_POAManager();
  pman->activate();

  Echo_i* myecho = new Echo_i();
  PortableServer::ObjectId_var myechoid = poa->activate_object(myecho);

  obj = myecho->_this();
  CORBA::String_var sior(orb_s->object_to_string(obj));

  if (argc != 2)
  {
    return 1;
  }

  pthread_t thread;
  pthread_create(&thread, 0, thread_proc, &orb_s);

  obj = orb_c->string_to_object(argv[1]);
  Echo_var echoref = Echo::_narrow(obj);
  if (CORBA::is_nil(echoref))
  {
    std::cerr << "Can't narrow reference to type Echo (or it was nil)." << std::endl;
    return 1;
  }

  hello(echoref, sior);

  sleep(10);

  orb_c->destroy();

  kill(getppid(), SIGINT);

  return 0;
}
