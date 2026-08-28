#include <iostream>
#include "echo.hpp"
#include "message.hpp"

class Shutdowner
{
public:
  Shutdowner(CORBA::ORB_var orb) noexcept;
  ~Shutdowner() noexcept;

private:
  void shutdown() noexcept;
  static void* thread_proc_(void* arg) noexcept;

  CORBA::ORB_var orb_;
  pthread_t thread_;
};


Shutdowner::Shutdowner(CORBA::ORB_var orb) noexcept
  : orb_(orb)
{
  pthread_create(&thread_, NULL, &thread_proc_, this);
}

Shutdowner::~Shutdowner() noexcept
{
  pthread_join(thread_, NULL);
  orb_ = 0;
}

void Shutdowner::shutdown() noexcept
{
  orb_->shutdown(true);
}

void* Shutdowner::thread_proc_(void* arg) noexcept
{
  static_cast<Shutdowner*>(arg)->shutdown();
  return NULL;
}

std::unique_ptr<Shutdowner> shut;

class Echo_i : public POA_Echo
{
public:
  Echo_i(CORBA::ORB_var orb) noexcept;

  virtual MessageHolder* echoString(MessageHolder* message) noexcept;
  virtual void shutdown() noexcept;

private:
  CORBA::ORB_var orb_;
};

Echo_i::Echo_i(CORBA::ORB_var orb) noexcept
  : orb_(orb)
{
}

MessageHolder* Echo_i::echoString(MessageHolder* message) noexcept
{
  CORBA::String_var message_string(message->get_message());
  return new MessageHolder_i(message_string);
}

void Echo_i::shutdown(void) noexcept
{
  shut.reset(new Shutdowner(orb_));
  orb_ = 0;
}

int main(int argc, char** argv)
{
  CORBA::ORB_var orb = CORBA::ORB_init(argc, argv, ORB_NAME);

  orb->register_value_factory("IDL:MessageHolder:1.0",
    CORBA::ValueFactoryBase_var(new MessageHolderFactory));

  CORBA::Object_var obj = orb->resolve_initial_references("RootPOA");
  PortableServer::POA_var poa = PortableServer::POA::_narrow(obj);

  Echo_i* myecho = new Echo_i(orb);
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

  shut.reset(0);

  return 0;
}
