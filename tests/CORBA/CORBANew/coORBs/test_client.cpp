#include <iostream>
#include <signal.h>
#include "echo.hpp"

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
  CORBA::ORB_var orb = CORBA::ORB_init(argc, argv);

  if (argc != 3)
  {
    return 1;
  }

  CORBA::Object_var obj1 = orb->string_to_object(argv[1]);
  Echo_var echoref1 = Echo::_narrow(obj1);
  if (CORBA::is_nil(echoref1))
  {
    std::cerr << "Can't narrow reference to type Echo (or it was nil)." << std::endl;
    return 1;
  }

  CORBA::Object_var obj2 = orb->string_to_object(argv[2]);
  Echo_var echoref2 = Echo::_narrow(obj2);
  if (CORBA::is_nil(echoref2))
  {
    std::cerr << "Can't narrow reference to type Echo (or it was nil)." << std::endl;
    return 1;
  }

  hello(echoref1);
  hello(echoref2);

  orb->destroy();

  kill(getppid(), SIGINT);

  return 0;
}
