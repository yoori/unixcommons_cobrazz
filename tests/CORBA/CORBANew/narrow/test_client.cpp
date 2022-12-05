#include <iostream>
#include <signal.h>
#include "echo.hpp"

static void
hello(Echo1_ptr e)
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

  if (argc != 2)
  {
    return 1;
  }

  CORBA::Object_var obj = orb->string_to_object(argv[1]);
  Echo1_var echoref = Echo1::_narrow(obj);
  if (CORBA::is_nil(echoref))
  {
    std::cerr << "Can't narrow reference to type Echo1 (or it was nil)." << std::endl;
    return 1;
  }

  hello(echoref);

  orb->destroy();

  kill(getppid(), SIGINT);

  return 0;
}
