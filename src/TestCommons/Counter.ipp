#include <iostream>

namespace TestCommons
{
  inline
  Counter::Counter() throw ()
    : success_(0), failure_(0)
  {
  }

  inline
  void
  Counter::print() const /*throw (eh::Exception)*/
  {
    print(std::cout);
  }

  inline
  void
  Counter::print(std::ostream& ostr) const /*throw (eh::Exception)*/
  {
    ostr
      << "Success: " << success_
      << " Fail: " << failure_
      << " Total: " << success_ + failure_
      << std::endl;
  }

  inline
  void
  Counter::success() throw ()
  {
    ++success_;
  }

  inline
  void
  Counter::failure() throw ()
  {
    ++failure_;
  }

  inline
  int
  Counter::succeeded() const throw ()
  {
    return success_;
  }

  inline
  int
  Counter::failed() const throw ()
  {
    return failure_;
  }
}
