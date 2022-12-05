/**
 * @file MTTesterLoggerTest.cpp
 */

#include <TestCommons/MTTester.hpp>

/// Multi-thread executable functor
struct Tester
{
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
  void
  operator ()() /*throw (Exception)*/;
};

void
Tester::operator ()() /*throw (Exception)*/
{
  throw Exception("Something wrong");
}


int
main() throw ()
{
  dup2(STDOUT_FILENO, STDERR_FILENO);

  try
  {
    std::cout << "MTTester logging exceptions test" << std::endl;

    Tester test;
    TestCommons::MTTester<Tester&> mt_tester(test, 5);

    mt_tester.run(20, 0, 20);
    std::cout << "SUCCESS" << std::endl;
    return 0;
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << ex.what() << std::endl;
  }
  catch (...)
  {
    std::cerr << "unknown exception" << std::endl;
  }

  return -1;
}
