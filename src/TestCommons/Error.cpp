#include <TestCommons/Error.hpp>


namespace TestCommons
{
  void
  Errors::add(const String::SubString& error, bool write) throw ()
  {
    try
    {
      {
        std::string key(error.data(), error.size());
        Sync::PosixGuard guard(mutex_);
        errors_[key]++;
      }

      if (write)
      {
        std::cerr << error << std::endl;
      }
    }
    catch (...)
    {
    }
  }

  void
  Errors::print() const throw ()
  {
    print(std::cout);
  }

  void
  Errors::print(std::ostream& ostr) const throw ()
  {
    Sync::PosixGuard guard(mutex_);

    if (errors_.empty())
    {
      ostr << "    None" << std::endl;
    }
    else
    {
      for (AllErrors::const_iterator itor(errors_.begin());
        itor != errors_.end(); ++itor)
      {
        ostr << "    " << itor->second << ": " << itor->first
          << std::endl;
      }
    }
  }
}
