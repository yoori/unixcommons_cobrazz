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

#ifdef ORB_OMNI
  omniORB::setClientCallTimeout(1000);
#endif

#ifdef ORB_MICO
  std::cerr << "Client timeouts not supported" << std::endl;
#endif

#ifdef ORB_TAO
  CORBA::Object_var object =
    orb->resolve_initial_references("ORBPolicyManager");

  CORBA::PolicyManager_var policy_manager =
    CORBA::PolicyManager::_narrow(object.in ());

  TimeBase::TimeT timeout = 10000000;
  CORBA::Any timeout_as_any;
  timeout_as_any <<= timeout;

  CORBA::Policy_var policy =
    orb->create_policy(Messaging::RELATIVE_RT_TIMEOUT_POLICY_TYPE,
      timeout_as_any);
  CORBA::PolicyList policy_list(1);
  policy_list.length(1);
  policy_list[0] = policy;

  policy_manager->set_policy_overrides(policy_list, CORBA::ADD_OVERRIDE);
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
