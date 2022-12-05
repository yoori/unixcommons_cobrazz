#include <iostream>
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

void
hello(CORBA::Object_ptr obj)
{
  Echo_var e = Echo::_narrow(obj);

  if (CORBA::is_nil(e))
  {
    std::cerr << "cannot invoke on a nil object reference." << std::endl;
    return;
  }

  CORBA::String_var src = (const char*) "Hello!";
  CORBA::String_var dest;

  dest = e->echoString(src);

  std::cerr << "I said, \"" << src << "\"." <<
    " The Object said, \"" << dest <<"\"" << std::endl;
}

int
main(int argc, char** argv)
{
  CORBA::ORB_ptr orb = CORBA::ORB_init(argc, argv, ORB_NAME);

  CORBA::Object_var obj = orb->resolve_initial_references("RootPOA");
  PortableServer::POA_var poa = PortableServer::POA::_narrow(obj);

  Echo_i* myecho = new Echo_i();
  PortableServer::ObjectId_var myechoid = poa->activate_object(myecho);

  Echo_var myechoref = myecho->_this();
  myecho->_remove_ref();

  PortableServer::POAManager_var pman = poa->the_POAManager();
  pman->activate();

  hello(myechoref);

  orb->destroy();
  return 0;
}
