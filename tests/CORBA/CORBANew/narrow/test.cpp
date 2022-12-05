#include <iostream>
#include "echo.hpp"

class Echo_i : virtual public POA_Echo1, virtual public POA_Echo2
{
public:
  virtual char*
  echoString(const char* message) throw ();
  virtual CORBA::Long
  echoLong(CORBA::Long message) throw ();

    virtual void invoke (CORBA::StaticServerRequest_ptr req)
    {
      if (!POA_Echo1::dispatch(req))
      {
        POA_Echo2::invoke(req);
      }
    }
    virtual CORBA::Boolean _is_a (const char *type)
    {
      return POA_Echo1::_is_a(type) || POA_Echo2::_is_a(type);
    }
    virtual CORBA::InterfaceDef_ptr _get_interface ()
    {
      try
      {
        return POA_Echo1::_get_interface();
      }
      catch (...)
      {
        return POA_Echo2::_get_interface();
      }
    }
    virtual CORBA::RepositoryId _primary_interface (const PortableServer::ObjectId & id, PortableServer::POA_ptr poa)
    {
      return POA_Echo2::_primary_interface(id, poa);
    }

    virtual void * _narrow_helper (const char *id)
    {
      void* ptr = POA_Echo1::_narrow_helper(id);
      return ptr ? ptr : POA_Echo2::_narrow_helper(id);
    }
    /*static Echo_i * _narrow (PortableServer::Servant serv)
    {
      void* ptr = POA_Echo1::_narrow(serv);
      if (!ptr)
      {
        ptr =  POA_Echo2::_narrow(serv);
      }
      return ptr ? this : 0;
    }*/
      class stub : public virtual Echo1_stub_clp, public virtual Echo2_stub_clp
      {
      public:
        stub(PortableServer::POA_ptr poa, CORBA::Object_ptr object)
          : CORBA::Object(*object), PortableServer::StubBase(poa),
            Echo1_stub_clp(poa, object), Echo2_stub_clp(poa, object)
        {
        }
        virtual void *_narrow_helper(const char *id)
        {
          void* ptr = Echo1_stub_clp::_narrow_helper(id);
          return ptr ? ptr : Echo2_stub_clp::_narrow_helper(id);
        }

      };
    virtual CORBA::Object_ptr _make_stub (PortableServer::POA_ptr poa, CORBA::Object_ptr object)
    {
      return new stub(poa, object);
    }

};

char*
Echo_i::echoString(const char* message) throw ()
{
  return CORBA::string_dup(message);
}

CORBA::Long
Echo_i::echoLong(CORBA::Long message) throw ()
{
  return message;
}

int
main(int argc, char** argv)
{
  CORBA::ORB_var orb = CORBA::ORB_init(argc, argv);

  CORBA::Object_var obj = orb->resolve_initial_references("RootPOA");
  PortableServer::POA_var poa = PortableServer::POA::_narrow(obj);

  POA_Echo2* myecho = new Echo_i();
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
