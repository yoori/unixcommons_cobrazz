// ApachePorts.hpp
#ifndef _APACHE_PORTS_HPP_INCLUDED_
#define _APACHE_PORTS_HPP_INCLUDED_

#include <string>
#include <cstdio>
#include <Generics/Uncopyable.hpp>
#include <Stream/MemoryStream.hpp>
#include <eh/Exception.hpp>

/**
 * Class ApachePorts provide access for Apache server
 * ports, that had been got from test.config.
 * Client will used ports number to connect to server
 * and will testing on these ports.
 */ 
class ApachePorts : Generics::Uncopyable
{
public:
  DECLARE_EXCEPTION(InvalidPortRequested, eh::DescriptiveException);

  /**
   * @param shift positive value to shift from base_port value
   * @return port number as integer
   */
  static int
  get_port(std::size_t shift)
    /*throw (InvalidPortRequested)*/;

  /**
   * @param shift positive value to shift from base_port value
   * @return port number as string
   */
  static std::string
  get_port_string(std::size_t shift)
    /*throw (InvalidPortRequested)*/;

private:
  /**
   * Read environment variable USER_BASE_PORT
   * @return USER_BASE_PORT value if exist, or default value
   */
  static int
  get_base_port_() /*throw (InvalidPortRequested)*/;

  /// Store USER_BASE_PORT value at run time.
  static int base_port_;
};

//
// Inlines
//

#endif // _APACHE_PORTS_HPP_INCLUDED_
