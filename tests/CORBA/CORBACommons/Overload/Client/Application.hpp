#ifndef CORBA_OVERLOAD_TEST_APPLICATION_HPP
#define CORBA_OVERLOAD_TEST_APPLICATION_HPP

#include <eh/Exception.hpp>

class Application
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
    
  void
  run(int argc, char* argv[]) /*throw(Exception, eh::Exception)*/;
};

#endif /* CORBA_OVERLOAD_TEST_APPLICATION_HPP */
