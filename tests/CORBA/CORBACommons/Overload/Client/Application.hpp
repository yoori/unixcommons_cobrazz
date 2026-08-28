#pragma once

#include <eh/Exception.hpp>

class Application
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  void run(int argc, char* argv[]) /*throw(Exception, eh::Exception)*/;
};
