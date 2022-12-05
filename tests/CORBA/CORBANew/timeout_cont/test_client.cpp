#include <iostream>
#include <sstream>
#include <list>
#include <signal.h>
#include "TestInt.hpp"

#define ORBRUN_

#if 0
#ifdef ORB_TAO
class NonBlockFlushingStrategy : public TAO_Flushing_Strategy
{
public:
  virtual int
  schedule_output(TAO_Transport* /*transport*/)
  {
    return MUST_FLUSH;
  }
  virtual int
  cancel_output(TAO_Transport* transport)
  {
    return 0;
  }
  virtual int
  flush_message(TAO_Transport* transport, TAO_Queued_Message* /*msg*/,
    ACE_Time_Value* max_wait_time)
  {
    return transport->handle_output(max_wait_time);
  }
  virtual int
  flush_transport(TAO_Transport* transport, ACE_Time_Value* max_wait_time)
  {
    return transport->handle_output(max_wait_time);
  }
};
#endif
#endif


volatile _Atomic_word total;

static void
hello(TestInt_ptr test_int)
{
  unsigned int PARAM_LEN = ::rand() % 10/*000*/;
  OctetSeq param;
  param.length(PARAM_LEN);
  for (CORBA::ULong i = 0; i < PARAM_LEN; ++i)
  {
    param[i] = i % 256;
  }

  test_int->oneway_test(total, param);
}

void*
thread_func(void* arg)
{
  int i = 0;
  for (CORBA::ULong count = 0; count < 10000; count++)
  {
    try
    {
      hello(TestInt_ptr(arg));
      int cur = __gnu_cxx::__exchange_and_add(&total, 1);
      i++;
      if (!(cur % 1000))
      {
        std::ostringstream ostr;
        ostr << i << " " << cur << "\n";
        const std::string& str = ostr.str();
        write(STDOUT_FILENO, str.data(), str.size());
      }
    }
    catch (...)
    {
      std::cout << "Exception caught\n";
    }
  }
  return NULL;
}

void*
thread_func3(void* arg)
{
  CORBA::ORB_ptr(arg)->run();
  std::cout << "ORB::run() ended\n";
  return NULL;
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
  CORBA::Object_var object = orb->resolve_initial_references("PolicyCurrent");

  CORBA::PolicyCurrent_var policy_current =
    CORBA::PolicyCurrent::_narrow(object.in ());

  TimeBase::TimeT timeout = 10000000;
  CORBA::Any timeout_as_any;
  timeout_as_any <<= timeout;

  CORBA::Policy_var policy =
    orb->create_policy(Messaging::RELATIVE_RT_TIMEOUT_POLICY_TYPE,
      timeout_as_any);
  CORBA::PolicyList policy_list(1);
  policy_list.length(1);
  policy_list[0] = policy;

  policy_current->set_policy_overrides(policy_list, CORBA::ADD_OVERRIDE);
#endif

  if (argc != 2)
  {
    return 1;
  }

  CORBA::Object_var obj = orb->string_to_object(argv[1]);
  TestInt_var echoref = TestInt::_narrow(obj);
  if (CORBA::is_nil(echoref))
  {
    std::cerr << "Can't narrow reference to type TestInt (or it was nil)." << std::endl;
    return 1;
  }

  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, 512 * 1024);

  std::list<pthread_t> threads;

  for (int i = 0; i < 100; i++)
  {
    pthread_t id;
    if (pthread_create(&id, &attr, thread_func, echoref) >= 0)
    {
      threads.push_back(id);
    }
  }

  pthread_attr_destroy(&attr);

#ifdef ORBRUN
  pthread_t orb_id;
  pthread_create(&orb_id, NULL, thread_func3, orb.in());
#endif

  while (!threads.empty())
  {
    pthread_join(threads.front(), NULL);
    threads.pop_front();
  }

#ifdef ORBRUN
  orb->shutdown();
  pthread_join(orb_id, NULL);
#endif

  orb->destroy();

#if 0
  sleep(60);
#endif

  kill(getppid(), SIGINT);

  return 0;
}
