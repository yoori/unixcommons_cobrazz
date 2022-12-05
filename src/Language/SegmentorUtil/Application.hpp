#ifndef LANGUAGE_SEGMENTOR_UTIL_APPLICATION_HPP
#define LANGUAGE_SEGMENTOR_UTIL_APPLICATION_HPP

#include <eh/Exception.hpp>

class Application
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  void
  run(int argc, char* argv[])
    /*throw (Exception, eh::Exception)*/;
};

#endif
