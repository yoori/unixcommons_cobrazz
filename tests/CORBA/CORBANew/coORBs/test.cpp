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

void*
thread_proc(void* arg)
{
  (*static_cast<CORBA::ORB_var*>(arg))->run();
  return 0;
}

int
main()
{
#ifdef ORB_OMNI
  char ENDPOINT[] = "-ORBendPoint";
  char ADDR1[] = "giop:tcp:localhost:1025";
  char ADDR2[] = "giop:tcp:localhost:1026";
#endif
#ifdef ORB_MICO
  char ENDPOINT[] = "-ORBIIOPAddr";
  char ADDR1[] = "inet:localhost:1025";
  char ADDR2[] = "inet:localhost:1026";
#endif
#ifdef ORB_TAO
  char ENDPOINT[] = "-ORBEndpoint";
  char ADDR1[] = "iiop://localhost:1025";
  char ADDR2[] = "iiop://localhost:1026";
#endif
  char* argv1[] = { "", ENDPOINT, ADDR1, NULL };
  int argc1 = sizeof(argv1) / sizeof(*argv1) - 1;
  char* argv2[] = { "", ENDPOINT, ADDR2, NULL };
  int argc2 = sizeof(argv2) / sizeof(*argv2) - 1;
  CORBA::ORB_var orb1 = CORBA::ORB_init(argc1, argv1, ORB_NAME "_1");
  CORBA::ORB_var orb2 = CORBA::ORB_init(argc2, argv2, ORB_NAME "_2");

#ifdef ORB_OMNI
  CORBA::Object_var obj1 = orb1->resolve_initial_references("omniINSPOA");
  CORBA::Object_var obj2 = orb2->resolve_initial_references("omniINSPOA");
#else
  CORBA::Object_var obj1 = orb1->resolve_initial_references("RootPOA");
  CORBA::Object_var obj2 = orb2->resolve_initial_references("RootPOA");
#endif
  PortableServer::POA_var root_poa1 = PortableServer::POA::_narrow(obj1);
  PortableServer::POA_var root_poa2 = PortableServer::POA::_narrow(obj2);
  PortableServer::POAManager_var pman1 = root_poa1->the_POAManager();
  PortableServer::POAManager_var pman2 = root_poa2->the_POAManager();
  pman1->activate();
  pman2->activate();

  Echo_i* myecho = new Echo_i();
#ifdef ORB_TAO
  root_poa1->activate_object(myecho);
  root_poa2->activate_object(myecho);

  CORBA::String_var ior_string1 = orb1->object_to_string(myecho->_this());
  CORBA::String_var ior_string2 = orb2->object_to_string(myecho->_this());

  CORBA::Object_var tobj1 = orb1->resolve_initial_references("IORTable");
  CORBA::Object_var tobj2 = orb2->resolve_initial_references("IORTable");
  IORTable::Table_var table1 = IORTable::Table::_narrow(tobj1.in());
  IORTable::Table_var table2 = IORTable::Table::_narrow(tobj2.in());

  table1->bind("Echo", ior_string1.in());
  table2->bind("Ohce", ior_string2.in());
#else
#ifdef ORB_OMNI
  PortableServer::POA_var poa1 = root_poa1;
  PortableServer::POA_var poa2 = root_poa2;
#else
  CORBA::PolicyList pl1;
  CORBA::PolicyList pl2;
  pl1.length(2);
  pl2.length(2);
  pl1[0] = root_poa1->create_lifespan_policy(PortableServer::PERSISTENT);
  pl2[0] = root_poa2->create_lifespan_policy(PortableServer::PERSISTENT);
  pl1[1] = root_poa1->create_id_assignment_policy(PortableServer::USER_ID);
  pl2[1] = root_poa2->create_id_assignment_policy(PortableServer::USER_ID);

  PortableServer::POA_var poa1 = root_poa1->create_POA("CustomPoa", pman1, pl1);
  PortableServer::POA_var poa2 = root_poa2->create_POA("CustomPoa", pman2, pl2);
#endif

  PortableServer::ObjectId_var oid1 =
    PortableServer::string_to_ObjectId("Echo");
  PortableServer::ObjectId_var oid2 =
    PortableServer::string_to_ObjectId("Ohce");

  poa1->activate_object_with_id(oid1, myecho);
  poa2->activate_object_with_id(oid2, myecho);
#endif

  if (!fork())
  {
#ifdef ORB_MICO
    std::string loc1 = "corbaloc::localhost:1025/Default/CustomPoa/Echo";
    std::string loc2 = "corbaloc::localhost:1026/Default/CustomPoa/Ohce";
#else
    std::string loc1 = "corbaloc::localhost:1025/Echo";
    std::string loc2 = "corbaloc::localhost:1026/Ohce";
#endif
    //execl("./test_client", "./test_client", (const char*)sior, NULL);
    execl("./test_client", "./test_client", loc1.c_str(), loc2.c_str(), NULL);
  }
  myecho->_remove_ref();

  pthread_t thread;
  pthread_create(&thread, 0, thread_proc, &orb2);
  orb1->run();

  return 0;
}
