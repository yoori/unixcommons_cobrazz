#include <iostream>
#include <vector>
#include "echo.hpp"

class Echo_i : public POA_Echo
{
public:
  Echo_i(CORBA::ORB_ptr orb);
  virtual char*
  echoString(const char* message, const char* ior) throw ();

private:
  CORBA::ORB_ptr orb_;
};

Echo_i::Echo_i(CORBA::ORB_ptr orb)
  : orb_(orb)
{
}

char*
Echo_i::echoString(const char* message, const char* ior) throw ()
{
  std::cerr << "Server!\n" << ior << std::endl;
  sleep(5);
  CORBA::Object_var obj = orb_->string_to_object(ior);
  Echo_var echoref = Echo::_narrow(obj);
  if (CORBA::is_nil(echoref))
  {
    std::cerr << "Can't narrow reference to type Echo (or it was nil)." << std::endl;
    return 0;
  }

  return echoref->echoString(message, "");
}

int
main(int argc, char** argv)
{
  //char ADDR[] = "iiop://0.0.0.0:1026/ssl_port=1025";
  char ADDR[] = "iiop://0.0.0.0:1026";

  std::vector<char*> args;

  args.resize(argc);
  std::copy(argv, argv + argc, args.begin());
  args.push_back("-ORBSvcConf");
  args.push_back("server_1.conf");
  args.push_back("-ORBEndpoint");
  args.push_back(ADDR);
  args.push_back(0);
  int argsc = args.size() - 1;
  CORBA::ORB_var orb_s = CORBA::ORB_init(argsc, &args[0], ORB_NAME "_1");

  args.resize(argc);
  std::copy(argv, argv + argc, args.begin());
  args.push_back("-ORBSvcConf");
  args.push_back("client_2.conf");
  args.push_back(0);
  argsc = args.size() - 1;
  CORBA::ORB_var orb_c = CORBA::ORB_init(argsc, &args[0], ORB_NAME "_2");

  CORBA::Object_var obj = orb_s->resolve_initial_references("RootPOA");
  PortableServer::POA_var poa = PortableServer::POA::_narrow(obj);

  Echo_i* myecho = new Echo_i(orb_c);
  PortableServer::ObjectId_var myechoid = poa->activate_object(myecho);

  obj = myecho->_this();
  CORBA::String_var sior(orb_s->object_to_string(obj));
#if 1
  if (!fork())
  {
    execl("./test_client", "./test_client", (const char*)sior, NULL);
  }
#else
  puts(sior);
#endif
  myecho->_remove_ref();

  PortableServer::POAManager_var pman = poa->the_POAManager();
  pman->activate();

  orb_s->run();

  return 0;
}
