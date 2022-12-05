#include <iostream>
#include <signal.h>
#include <dirent.h>
#include "echo.hpp"

class Echo_i : public POA_Echo
{
public:
  virtual char*
  echoString(const char* message) throw ();
};

char*
Echo_i::echoString(const char* message) throw ()
{
  return CORBA::string_dup(message);
}

int
main(int argc, char** argv)
{
  CORBA::ORB_ptr orb = CORBA::ORB_init(argc, argv, ORB_NAME);

  CORBA::Object_var obj = orb->resolve_initial_references("RootPOA");
  PortableServer::POA_var poa = PortableServer::POA::_narrow(obj);

  Echo_i* myecho = new Echo_i();
  PortableServer::ObjectId_var myechoid = poa->activate_object(myecho);

  obj = myecho->_this();
  CORBA::String_var sior(orb->object_to_string(obj));
  if (!fork())
  {
    execl("./test_client", "./test_client", (const char*)sior, NULL);
  }
  myecho->_remove_ref();

  PortableServer::POAManager_var pman = poa->the_POAManager();
  pman->activate();

  orb->run();

  return 0;
}
