#include <memory>
#include <string>
#include <iostream>
#include <algorithm>

#include <eh/Exception.hpp>
#include <Generics/ArrayAutoPtr.hpp>
#include <Generics/Rand.hpp>
#include <Generics/Time.hpp>

#include <TestCommons/MTTester.hpp>

using namespace Generics;

namespace
{
  DECLARE_EXCEPTION(TestException, eh::DescriptiveException);
}

class MultiThreadPerformanceTest
{
public:
  MultiThreadPerformanceTest(std::size_t meters,
    std::size_t buffers_amount)
    throw ();

  void
    operator()() /*throw (eh::Exception)*/;

private:
  const std::size_t METERS_;
  const std::size_t BUFFERS_AMOUNT_;
};

MultiThreadPerformanceTest::MultiThreadPerformanceTest(
  std::size_t meters,
  std::size_t buffers_amount) throw ()
  : METERS_(meters),
    BUFFERS_AMOUNT_(buffers_amount)
{
}

volatile _Atomic_word g_do_functors_counter_(0);

void
MultiThreadPerformanceTest::operator()() /*throw (eh::Exception)*/
{
  __gnu_cxx::__atomic_add(&g_do_functors_counter_, 1);
  for (std::size_t j = 0; j < METERS_; ++j)
  {
    for (std::size_t i = 0; i < BUFFERS_AMOUNT_; ++i)
    {
    }
  }
}

void
do_performance_test(std::size_t threads) /*throw (eh::Exception)*/
{
  std::cout << "\n\tSTART performance metering for " << threads
    << " threads." << std::endl;

//  MultiThreadPerformanceTest mtt(1, 100);

//  TestCommons::MTTester<MultiThreadPerformanceTest&>
//    mt_tester(mtt, threads);

  CPUTimer timer;

  for (std::size_t i = 0;
    i < 16;
    ++i)
  {
    MultiThreadPerformanceTest mtt(1, 100);
    TestCommons::MTTester<MultiThreadPerformanceTest&>
      mt_tester(mtt, threads);

    g_do_functors_counter_ = 0;
    timer.start();
    mt_tester.run(threads, 0, threads);
    timer.stop();

    std::cout << "Functor executed " << g_do_functors_counter_
      << " times." << std::endl;
    if (g_do_functors_counter_ != static_cast<int>(threads))
    {
      throw TestException("Not equal");
    }
    g_do_functors_counter_ = 0;
  }

}

int
main()
{
  std::cout << "MTTester test started" << std::endl;
  
  try
  {
    for (std::size_t i = 1; i < 64; ++i)
    {
      do_performance_test(i);
    }
    
    std::cout << "Test complete" << std::endl;
  }
  catch (const eh::Exception& e)
  {
    std::cerr << "FAIL: " << e.what() << std::endl;
  }
  catch (...)
  {
    std::cerr << "unknown exception" << std::endl;
  }

  return 0;
}
