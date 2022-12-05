#include <iostream>
#include <vector>
#include <signal.h>
#include <sys/wait.h>
#include "echo.hpp"

#define __USE_GNU
#include <dlfcn.h>

static void
hello(Echo_ptr e)
{
  CORBA::String_var src = (const char*) "Hello!";
  CORBA::String_var dest = e->echoString(src);

  /*std::cout << "I said, \"" << src << "\"." << std::endl
       << "The Echo object replied, \"" << dest <<"\"." << std::endl;*/
}

class Parallel
{
public:
  Parallel(const CORBA::ORB_var& orb, const char* ior, int threads)
    : orb_(orb), ior_(ior), flag_(false)
  {
    threads_.reserve(threads);
    while (threads-- > 0)
    {
      pthread_t thread;
      pthread_create(&thread, 0, thread_proc_, this);
      threads_.push_back(thread);
    }
  }
  ~Parallel()
  {
    flag_ = true;
    while (!threads_.empty())
    {
      pthread_t thread = threads_.back();
      threads_.pop_back();
      pthread_join(thread, 0);
    }
  }

private:
  void
  thread_proc_()
  {
    CORBA::Object_var obj = orb_->string_to_object(ior_);
    Echo_var echoref = Echo::_narrow(obj);
    if (CORBA::is_nil(echoref))
    {
      std::cerr << "Can't narrow reference to type Echo (or it was nil)." << std::endl;
      return;
    }

    while (!flag_)
    {
      hello(echoref);
    }
  }

  static void*
  thread_proc_(void* arg) throw ()
  {
    try
    {
      static_cast<Parallel*>(arg)->thread_proc_();
    }
    catch (...)
    {
    }
    return 0;
  }

  CORBA::ORB_var orb_;
  const char* ior_;
  volatile sig_atomic_t flag_;
  std::vector<pthread_t> threads_;
};

extern "C" int
connect(int sockfd, const struct sockaddr* serv_addr, socklen_t addrlen)
{
  typedef int (*Connect)(int, const struct sockaddr*, socklen_t);
  static Connect con;

  if (!con)
  {
    con = reinterpret_cast<Connect>(dlsym(RTLD_NEXT, "connect"));
  }

  std::cout << "connect(2) called\n";

  return con(sockfd, serv_addr, addrlen);
}

int
main(int argc, char** argv)
{
  if (argc != 2)
  {
    return 1;
  }

  CORBA::ORB_var orb = CORBA::ORB_init(argc, argv);

  {
    Parallel parallel(orb, argv[1], 50);
    sleep(10);
  }

  orb->destroy();

  kill(getppid(), SIGINT);

  return 0;
}
