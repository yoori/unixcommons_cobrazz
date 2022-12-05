#include <iostream>
#include <vector>
#include <signal.h>
#include <sys/wait.h>
#include "echo.hpp"

const int PROCESSES = 1;
const int THREADS = 30;
const int TIMEOUT = 5;

static void
hello(Echo_ptr e)
{
  CORBA::String_var src = (const char*) "Hello!";
  time_t sent_server = e->echoString(time(NULL), src);
  time_t received_client = time(NULL);
  if (received_client > sent_server + 1)
  {
    printf("Client: %u\n",
      static_cast<unsigned>(received_client - sent_server));
  }
  if (!(rand() & 0xFF))
  {
    printf("Shutting it down!\n");
    e->shutdown();
  }
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
      try
      {
        hello(echoref);
      }
      catch (const CORBA::Exception&)
      {
      }
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

class Fork
{
public:
  Fork(int n, int argc, char* argv[])
  {
    pids.reserve(n);
    for (int i = 0; i < n; i++)
    {
      if (pid_t pid = fork())
      {
        pids.push_back(pid);
      }
      else
      {
        srand(time(NULL) ^ getpid());
        {
          CORBA::ORB_var orb = CORBA::ORB_init(argc, argv);
#ifdef ORB_MICO
          {
            CORBA::DomainManager_var dm;
            orb->get_default_domain_manager(dm);
          }
#endif

          {
            Parallel parallel(orb, argv[1], THREADS);
            sleep(TIMEOUT);
          }

          orb->destroy();
        }
        exit(0);
      }
    }
  }

  ~Fork()
  {
    while (!pids.empty())
    {
      pid_t pid = pids.back();
      pids.pop_back();
      waitpid(pid, 0, 0);
    }
  }

private:
  std::vector<pid_t> pids;
};

int
main(int argc, char** argv)
{
#if 0
  int fd = open("/dev/null", O_WRONLY);
  dup2(fd, STDOUT_FILENO);
  dup2(fd, STDERR_FILENO);
  close(fd);
#endif

  if (argc != 2)
  {
    return 1;
  }

  sleep(1);

  {
    Fork fork(PROCESSES, argc, argv);
  }

  sleep(5);

  kill(getppid(), SIGINT);

  return 0;
}
