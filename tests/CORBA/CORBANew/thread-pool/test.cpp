#include <iostream>
#include <sstream>
#include <signal.h>
#include <dirent.h>
#include <vector>
#include "echo.hpp"

#include <Generics/AtomicInt.hpp>

class Shutdowner
{
public:
  Shutdowner(CORBA::ORB_var orb) throw ();
  ~Shutdowner() throw ();

private:
  void
  shutdown() throw ();
  static void*
  thread_proc_(void* arg) throw ();

  CORBA::ORB_var orb_;
  pthread_t thread_;
};


Shutdowner::Shutdowner(CORBA::ORB_var orb) throw ()
  : orb_(orb)
{
  pthread_create(&thread_, NULL, &thread_proc_, this);
}

Shutdowner::~Shutdowner() throw ()
{
  pthread_join(thread_, NULL);
  orb_ = 0;
}

void
Shutdowner::shutdown() throw ()
{
  orb_->shutdown(true);
}

void*
Shutdowner::thread_proc_(void* arg) throw ()
{
  static_cast<Shutdowner*>(arg)->shutdown();
  return NULL;
}

std::unique_ptr<Shutdowner> shut;

static Generics::AtomicInt req;

class Echo_i : public POA_Echo
{
public:
  Echo_i(CORBA::ORB_var orb) throw ();
  virtual CORBA::Long
  echoString(CORBA::Long sent_client, const char* message) throw ();
  void
  shutdown() throw ();

private:
  pthread_mutex_t mutex_;
  CORBA::ORB_var orb_;
};

Echo_i::Echo_i(CORBA::ORB_var orb) throw ()
  : orb_(orb)
{
  pthread_mutex_init(&mutex_, 0);
}

CORBA::Long
Echo_i::echoString(CORBA::Long sent_client, const char* message) throw ()
{
  ++req;
  time_t received_server = time(NULL);
  if (received_server > sent_client + 1)
  {
    std::ostringstream ostr;
    ostr << pthread_self() << " Server " <<
      ": " << static_cast<unsigned>(received_server - sent_client) << "\n";
    const std::string& str = ostr.str();
    write(1, str.data(), str.length());
  }

  CORBA::String_var dup = CORBA::string_dup(message);

  return time(NULL);
}

void
Echo_i::shutdown() throw ()
{
  pthread_mutex_lock(&mutex_);
  if (orb_)
  {
    shut.reset(new Shutdowner(orb_));
    orb_ = 0;
  }
  pthread_mutex_unlock(&mutex_);
}

class ThreadCounter
{
public:
  ThreadCounter()
    : flag_(false)
  {
    pthread_create(&thread_, 0, thread_proc_, const_cast<sig_atomic_t*>(&flag_));
  }
  ~ThreadCounter()
  {
    flag_ = true;
    pthread_join(thread_, 0);
  }

  static int
  get_number_of_threads()
  {
    char buf[64];
    snprintf(buf, sizeof(buf), "/proc/%u/task", (unsigned)getpid());
    DIR* dir = opendir(buf);
    if (!dir)
    {
      return 0;
    }
    int threads = -2;
    while (readdir(dir))
    {
      threads++;
    }
    closedir(dir);
    return threads;
  }

  static void*
  thread_proc_(void* arg)
  {
    _Atomic_word last = 0;
    do
    {
      int cur = req.exchange_and_add(0);
      std::ostringstream ostr;
      ostr << "Number of threads: " << get_number_of_threads() << " " <<
        cur - last << std::endl;
      const std::string& str = ostr.str();
      write(1, str.data(), str.length());
      last = cur;
      sleep(1);
    }
    while (arg && !*static_cast<volatile sig_atomic_t*>(arg));
    return 0;
  }

private:
  pthread_t thread_;
  volatile sig_atomic_t flag_;
};

#ifdef ORB_TAO
void*
thread_proc(void* arg)
{
  (*static_cast<CORBA::ORB_var*>(arg))->run();
  std::cout << "Thread terminated" << std::endl;
  return 0;
}
#endif

int
main(int argc, char** argv)
{
  {
  ThreadCounter counter;

#ifdef ORB_TAO
  int LIMIT = 20;
  //LIMIT = 0;
  //LIMIT = 5;
#else
  char LIMIT[] = "20";
#endif
#ifdef ORB_OMNI
  const char* options[][2] =
    {
      { "threadPerConnectionPolicy", "0" },
      { "maxServerThreadPoolSize", LIMIT },
      { 0, 0 }
    };
  CORBA::ORB_var orb = CORBA::ORB_init(argc, argv, ORB_NAME, options);
#else
  std::vector<char*> args;
  args.resize(argc);
  std::copy(argv, argv + argc, args.begin());
#ifdef ORB_MICO
  args.push_back("-ORBThreadPool");
  args.push_back("-ORBRequestLimit");
  args.push_back(LIMIT);
#else
#if 0
  args.push_back("-ORBSvcConfDirective");
  args.push_back("static Server_Strategy_Factory \"-ORBConcurrency thread-per-connection\"");
#endif
#if 0
  args.push_back("-ORBSvcConfDirective");
  args.push_back("static Advanced_Resource_Factory \"-ORBReactorType dev_poll\"");
#endif
#endif
  args.push_back(0);
  int argsc = args.size() - 1;
  CORBA::ORB_var orb = CORBA::ORB_init(argsc, &args[0], ORB_NAME);
#endif

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

#ifdef ORB_TAO
  //TAO_debug_level = 100;
  for (int i = 0; i < LIMIT; i++)
  {
    pthread_t thread;
    pthread_create(&thread, 0, thread_proc, &orb);
  }
#endif

#if 0
  sleep(20);
  orb->shutdown();
  sleep(10);
#else
  orb->run();
#endif

  shut.reset(0);
  }

  write(1, "Final ", 6);
  ThreadCounter::thread_proc_(0);

  return 0;
}
