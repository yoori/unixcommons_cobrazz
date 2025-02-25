#include <vector>
#include "Tests.hpp"
#include <HTTP/HttpAsyncPoolInternals.hpp>

const unsigned int REQUESTS_COUNT = 500;
const unsigned int SERVER_CONNECTIONS_COUNT = 20;
const unsigned int THREAD_CONNECTIONS_COUNT = 5;
const unsigned int THREADS_COUNT = 20;
const unsigned int POOLS_COUNT = 1;
const unsigned int UNITS_COUNT = 1;

const char NOTIFICATION_MSG[] = "///////////////////////////////////////////////\n"
                                " TO KNOW MORE ABOUT SCENARIOS RUN WITH \"help\""
                                "\n///////////////////////////////////////////////";

char hostname[HOST_NAME_MAX];
const int hostname_res = gethostname(hostname, sizeof(hostname));

void usage()
{
  std::cout << '\n' << BasicsTest01::scenario_descr()
            << '\n' << BasicsTest02::scenario_descr()
            << '\n' << BasicsTest03::scenario_descr()
            << '\n' << BasicsTest04::scenario_descr()
            << '\n' << RandomLoadingTest::scenario_descr()
            << '\n' << std::endl;
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
    tests_runner->activate_object();

    Sync::Semaphore finish_sem(0);
    typedef ReferenceCounting::List<PoliciesTestInterface_var> Tests;
    Tests tests;

    if (POOLS_COUNT > UNITS_COUNT ||
        THREADS_COUNT <= UNITS_COUNT)
    {
      return -1;
    }

    tests.push_back(PoliciesTestInterface_var(
      new BasicsTest01(finish_sem, servers)));
    tests_runner->enqueue_task(tests.back());
    tests.push_back(PoliciesTestInterface_var(
      new BasicsTest02(finish_sem, servers)));
    tests_runner->enqueue_task(tests.back());
    tests.push_back(PoliciesTestInterface_var(
      new BasicsTest03(finish_sem, servers)));
    tests_runner->enqueue_task(tests.back());
    tests.push_back(PoliciesTestInterface_var(
      new BasicsTest04(finish_sem, servers)));
    tests_runner->enqueue_task(tests.back());
    tests.push_back(PoliciesTestInterface_var(
      new RandomLoadingTest(finish_sem, servers)));
    tests_runner->enqueue_task(tests.back());

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
      (*itor)->print_stats(std::cout);
      (*itor)->print_errors(std::cerr);
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
