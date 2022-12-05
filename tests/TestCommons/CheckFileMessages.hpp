#ifndef TESTCOMMONS_CHECKFILEMESSAGES
#define TESTCOMMONS_CHECKFILEMESSAGES

#include <vector>
#include <string>
#include <fstream>

#include <time.h>

#include <eh/Exception.hpp>

#include <Generics/Time.hpp>
#include <Generics/DirSelector.hpp>


namespace TestCommons
{
  class CheckFileMessages
  {
  public:

    DECLARE_EXCEPTION(CheckException, eh::DescriptiveException);

    void
    add_message() /*throw (eh::Exception)*/;

    void
    check(std::string file, int size_span, int time_span)
      /*throw (eh::Exception, CheckException)*/;

  private:
    class FileNameComparer
    {
    public:
      FileNameComparer(const std::string& common) /*throw (eh::Exception)*/;

      bool
      operator ()(const std::string& left, const std::string& right)
        throw ();

    private:
      std::string common_;
    };

    typedef std::vector<time_t> Timestamps;

    static const time_t max_delay_;

    Timestamps timestamps_;
  };
}

#endif
