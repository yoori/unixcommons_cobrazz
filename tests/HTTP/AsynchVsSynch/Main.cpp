#include <vector>
#include "Tests.hpp"
#include <HTTP/HttpAsyncPoolInternals.hpp>

const unsigned int REQUESTS_COUNT = 5000;
const unsigned int SERVER_CONNECTIONS_COUNT = 20;
const unsigned int THREAD_CONNECTIONS_COUNT = 5;

const unsigned int THREADS_COUNT[] = {
  20,
  40
  };
const size_t THREADS_COUNT_SIZE =
  sizeof(THREADS_COUNT) / sizeof(THREADS_COUNT[0]);

const unsigned int POOLS_COUNT[] = {
  1,
  5,
  10
  };
const size_t POOLS_COUNT_SIZE =
  sizeof(POOLS_COUNT) / sizeof(POOLS_COUNT[0]);

const unsigned int UNITS_COUNT[] = {
  1,
  5,
  10
  };
const size_t UNITS_COUNT_SIZE =
  sizeof(UNITS_COUNT) / sizeof(UNITS_COUNT[0]);

const char NOTIFICATION_MSG[] = "///////////////////////////////////////////////\n"
                                " TO KNOW MORE ABOUT SCENARIOUS RUN WITH \"help\""
                                "\n///////////////////////////////////////////////";

char hostnamez[HOST_NAME_MAX];
const int hostnamez_res = gethostname(hostnamez, sizeof(hostnamez));

void usage()
{
  std::cout << "General AsynchVsSynch test params:\n"
            << "THREADS_COUNT = " << THREADS_COUNT
            << "\nREQUESTS_COUNT = " << REQUESTS_COUNT
            << "\n\n"
            << std::endl;
}

int
main(int argc, char* argv[])
{
  try
  {
    rlimit limit;
    if(getrlimit(RLIMIT_NOFILE, &limit) == 0)
    {
      limit.rlim_cur = limit.rlim_max;  
      setrlimit(RLIMIT_NOFILE, &limit);
    }

    if (argc != 1 && std::string(argv[1]) == "help")
    {
      usage();
      return 0;
    }

    std::vector<HTTP::HttpServer> servers;

    SimplePolicy_var policy(new SimplePolicy);
    Generics::TaskRunner_var tests_runner(new Generics::TaskRunner(policy, 1));

    Sync::Semaphore finish_sem(0);
    typedef ReferenceCounting::List<VSTestInterface_var> Tests;
    Tests tests;

    for (size_t thr_cnt = 0; thr_cnt < THREADS_COUNT_SIZE; ++thr_cnt)
    {
      bool first = true;
      for (size_t uni_cnt = 0; uni_cnt < UNITS_COUNT_SIZE; ++uni_cnt)
      {
        for (size_t pool_cnt = 0; pool_cnt < POOLS_COUNT_SIZE; ++pool_cnt)
        {
          if (POOLS_COUNT[pool_cnt] > UNITS_COUNT[uni_cnt] ||
              THREADS_COUNT[thr_cnt] <= UNITS_COUNT[uni_cnt])
          {
            continue;
          }

          tests.push_back(VSTestInterface_var(new CommonTest(
            finish_sem,
            THREADS_COUNT[thr_cnt],
            REQUESTS_COUNT,
            POOLS_COUNT[pool_cnt],
            UNITS_COUNT[uni_cnt],
            SERVER_CONNECTIONS_COUNT,
            THREAD_CONNECTIONS_COUNT,
            servers,
            true, !first/*Keep-Alive, Only-Asynch*/)));

          tests_runner->enqueue_task(tests.back());

          first = false;
        }
      }
    }

    tests_runner->activate_object();

    size_t tests_count = tests.size();
    for (size_t i = 0; i < tests_count; ++i)
    {
      finish_sem.acquire();
    }

    tests_runner->deactivate_object();
    tests_runner->wait_object();

    std::cout << NOTIFICATION_MSG << "\n\n";
    for (Tests::iterator itor(tests.begin()); itor != tests.end(); ++itor)
    {
      (*itor)->print_stat(std::cout);
      std::cout << std::endl;
    }

    tests_runner.reset();
    policy.reset();
  }
  catch (const eh::Exception& e)
  {
    std::cerr << "[ERROR]: main(2). eh::Exception caught: " <<
      e.what() << std::endl;

    return -1;
  }

  return 0;
}
