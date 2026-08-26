#include <iostream>

namespace TestCommons
{
  inline
  Counter::Counter() noexcept
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
  Counter::success() noexcept
  {
    ++success_;
  }

  inline
  void
  Counter::failure() noexcept
  {
    ++failure_;
  }

  inline
  int
  Counter::succeeded() const noexcept
  {
    return success_;
  }

  inline
  int
  Counter::failed() const noexcept
  {
    return failure_;
  }
}
