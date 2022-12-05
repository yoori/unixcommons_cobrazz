#include <iostream>
#include <sstream>
#include <assert.h>
#include <bits/atomicity.h>
#include "TestInt.hpp"

class TestInt_i : public POA_TestInt
{
public:
  TestInt_i() throw ();
  virtual void
  test(CORBA::Long number, const OctetSeq& in_seq) throw ();
  virtual void
  oneway_test(CORBA::Long number, const OctetSeq& in_seq) throw ();
private:
  volatile _Atomic_word total;
};

TestInt_i::TestInt_i() throw ()
  : total(0)
{
}

void
TestInt_i::test(CORBA::Long number, const OctetSeq& in_seq) throw ()
{
  unsigned sleep = 900 + rand() % 200;
  timeval tv = { sleep / 1000, (sleep % 1000) * 1000 };
  CORBA::ULong PARAM_LEN = in_seq.length();

  for (CORBA::ULong i = 0; i < PARAM_LEN; ++i)
  {
    assert(in_seq[i] == i % 256);
    //select(0, 0, 0, 0, &tv);
  }

  int cur = __gnu_cxx::__exchange_and_add(&total, 1);
  if (!(cur % 1000))
  {
    std::ostringstream ostr;
    ostr << "T" << cur << " " << number << "\n";
    const std::string& str = ostr.str();
    write(STDOUT_FILENO, str.data(), str.size());
  }
}

void
TestInt_i::oneway_test(CORBA::Long number, const OctetSeq& in_seq) throw ()
{
  test(number, in_seq);
}

int
main(int argc, char** argv)
{
#if 0
  std::cout << getpid() << std::endl;
  sleep(10);
#endif

  CORBA::ORB_var orb = CORBA::ORB_init(argc, argv, ORB_NAME);

  CORBA::Object_var obj = orb->resolve_initial_references("RootPOA");
  PortableServer::POA_var poa = PortableServer::POA::_narrow(obj);

  TestInt_i* myecho = new TestInt_i();
  PortableServer::ObjectId_var myechoid = poa->activate_object(myecho);

  obj = myecho->_this();
  CORBA::String_var sior(orb->object_to_string(obj));
  //puts(sior);
  if (!fork())
  {
#if 1
    execl("./test_client", "./test_client", (const char*)sior, NULL);
#else
    FILE* file = fopen(".gdbinit", "w");
    fprintf(file, "set args %s\ndirectory /home/konstantin_sadov/work/ACE_wrappers/ace\nbr main\nrun\n", (const char*) sior);
    //fprintf(file, "br 'TAO_Transport::send_message_shared_i(TAO_Stub*, TAO_Transport::TAO_Message_Semantics, ACE_Message_Block const*, ACE_Time_Value*)'\n");
    fprintf(file, "br 'TAO_Leader_Follower_Flushing_Strategy::flush_transport(TAO_Transport*, ACE_Time_Value*)'\n");
    fprintf(file, "br 'TAO_Leader_Follower_Flushing_Strategy::flush_message(TAO_Transport*, TAO_Queued_Message*, ACE_Time_Value*)'\n");
    fprintf(file, "cont\n");
    fclose(file);
    //execl("gdb", "gdb", "./test_client", NULL);
#endif
  }
  myecho->_remove_ref();

  PortableServer::POAManager_var pman = poa->the_POAManager();
  pman->activate();

  orb->run();

  return 0;
}
