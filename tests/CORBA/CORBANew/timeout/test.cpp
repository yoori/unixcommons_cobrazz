#include <iostream>
#include "echo.hpp"

class Echo_i : public POA_Echo
{
public:
  Echo_i() throw ();
  virtual char*
  echoString(const char* message) throw ();
private:
  unsigned sleep;
};

Echo_i::Echo_i() throw ()
  : sleep(10)
{
}

char*
Echo_i::echoString(const char* message) throw ()
{
  timeval tv = { sleep / 1000, (sleep % 1000) * 1000 };
  sleep *= 2;
  select(0, 0, 0, 0, &tv);
  return CORBA::string_dup(message);
}

int
main(int argc, char** argv)
{
  CORBA::ORB_var orb = CORBA::ORB_init(argc, argv, ORB_NAME);

  CORBA::Object_var obj = orb->resolve_initial_references("RootPOA");
  PortableServer::POA_var poa = PortableServer::POA::_narrow(obj);

  Echo_i* myecho = new Echo_i();
  PortableServer::ObjectId_var myechoid = poa->activate_object(myecho);

  obj = myecho->_this();
  CORBA::String_var sior(orb->object_to_string(obj));
  //puts(sior);
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
