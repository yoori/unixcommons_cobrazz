#include <iostream>
#include "echo.hpp"

#ifdef ORB_TAO
#define ADVANCED
#endif

#ifdef ADVANCED
#include <tao/EndpointPolicy/EndpointPolicy.h>
#include <tao/EndpointPolicy/IIOPEndpointValue_i.h>
#endif

class Echo_i : public POA_Echo
{
public:
  virtual void
  two_way() throw ();
  virtual void
  one_way() throw ();
};

void
Echo_i::two_way() throw ()
{
  std::cerr << "two_way\n";
}

void
Echo_i::one_way() throw ()
{
  std::cerr << "one_way\n";
}

int
main()
{
  //TAO_debug_level = 100;

#ifdef ORB_OMNI
  char ENDPOINT[] = "-ORBendPoint";
  char ADDR[] = "giop:tcp:localhost:1025";
#endif
#ifdef ORB_MICO
  char ENDPOINT[] = "-ORBIIOPAddr";
  char ADDR[] = "inet:localhost:1025";
#endif
#ifdef ORB_TAO
  char ENDPOINT[] = "-ORBEndpoint";
  char ADDR[] = "iiop://localhost:1025";
#endif
  char* argv[] = { "",
#ifdef TAO
    "-ORBSkipServiceConfigOpen",
#endif
#ifdef ADVANCED
#if 0
    "-ORBGestalt",
    "Local",
#endif
#if 0
    "-ORBSvcConfDirective",
    "dynamic FT_ClientService_Activate "
      "Service_Object * "
      "TAO_FT_ClientORB:_make_TAO_FT_ClientService_Activate() \"\"",
#endif
#if 0
    "-ORBSvcConfDirective",
    "remove FT_ClientService_Activate",
#endif
#endif
    ENDPOINT, ADDR, NULL };
  int argc = sizeof(argv) / sizeof(*argv) - 1;
  CORBA::ORB_var orb = CORBA::ORB_init(argc, argv, ORB_NAME);

#ifdef ORB_OMNI
  CORBA::Object_var obj = orb->resolve_initial_references("omniINSPOA");
#else
  CORBA::Object_var obj = orb->resolve_initial_references("RootPOA");
#endif
  PortableServer::POA_var root_poa = PortableServer::POA::_narrow(obj);
  PortableServer::POAManager_var pman = root_poa->the_POAManager();

#ifdef ORB_TAO

#ifdef ADVANCED
    {
      {
        PortableServer::POAManagerFactory_var poa_manager_factory =
          root_poa->the_POAManagerFactory();
        CORBA::PolicyList policies;
#if 0
        EndpointPolicy::EndpointValueBase_var endpoint =
          new IIOPEndpointValue_i("localhost", 1025);
        EndpointPolicy::EndpointList list;
        list.length(1);
        list[0] = endpoint;
        CORBA::Any policy_value;
        policy_value <<= list;
        CORBA::Policy_var policy = orb->create_policy(
          EndpointPolicy::ENDPOINT_POLICY_TYPE, policy_value);
        policies.length(1);
        policies[0] = policy;
#endif
        pman = poa_manager_factory->create_POAManager("Custom_POA_Manager", policies);
      }
      CORBA::PolicyList policies;
      CORBA::Policy_var policy0, policy1, policy2;
      policies.length(3);
      policies[0] = policy0 =
        root_poa->create_lifespan_policy(PortableServer::PERSISTENT);
      policies[1] = policy1 =
        root_poa->create_id_uniqueness_policy(PortableServer::MULTIPLE_ID);
      policies[2] = policy2 =
        root_poa->create_id_assignment_policy(PortableServer::USER_ID);
      root_poa = root_poa->create_POA("Custom_POA", pman, policies);
    }
#endif

  Echo_i* myecho = new Echo_i();

#ifdef ADVANCED
  PortableServer::ObjectId_var object_id(
    PortableServer::string_to_ObjectId("Object"));
  root_poa->activate_object_with_id(object_id, myecho);
#else
  root_poa->activate_object(myecho);
#endif

#ifdef ADVANCED
  CORBA::Object_var obj_ref = root_poa->id_to_reference(object_id);
  CORBA::String_var ior_string = orb->object_to_string(obj_ref);
#else
  CORBA::String_var ior_string = orb->object_to_string(myecho->_this());
#endif

  CORBA::Object_var tobj = orb->resolve_initial_references("IORTable");
  IORTable::Table_var table = IORTable::Table::_narrow(tobj.in());

  table->bind("Echo", ior_string.in());
#else
#ifdef ORB_OMNI
  PortableServer::POA_var poa = root_poa;
#else
  CORBA::PolicyList pl;
  pl.length(2);
  pl[0] = root_poa->create_lifespan_policy(PortableServer::PERSISTENT);
  pl[1] = root_poa->create_id_assignment_policy(PortableServer::USER_ID);

  PortableServer::POA_var poa = root_poa->create_POA("CustomPoa", pman, pl);
#endif

  PortableServer::ObjectId_var oid =
    PortableServer::string_to_ObjectId("Echo");

  Echo_i* myecho = new Echo_i();

  poa->activate_object_with_id(oid, myecho);
#endif

  pman->activate();

  myecho->_remove_ref();

  std::cerr << "Running\n";

  orb->run();

  return 0;
}
