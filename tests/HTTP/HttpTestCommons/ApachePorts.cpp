// ApachePorts.cpp

#include <stdlib.h>

#include "ApachePorts.hpp"

//
// class ApachePorts
//

int ApachePorts::base_port_ = 0;

inline
int
ApachePorts::get_base_port_() /*throw (InvalidPortRequested)*/
{
  if (base_port_ == 0)
  {
    char *p = getenv("USER_BASE_PORT");
    int port = 10000;
    if (p)
    {
      port = atoi(p);
    }
    if (port > 65535 || port <= 0)
    {
      Stream::Error ostr;
      ostr << "Incorrect base clients port value: USER_BASE_PORT="
        << (p? p : "null") << std::endl;
      throw InvalidPortRequested(ostr);
    }
    base_port_ = port;
  }
  return base_port_;
}

int
ApachePorts::get_port(std::size_t shift)
  /*throw (InvalidPortRequested)*/
{
  int port = ApachePorts::get_base_port_() + shift;
  if (port > 65535 || port <= 0)
  {
    Stream::Error ostr;
    ostr << "Incorrect clients port requested: USER_BASE_PORT="
      << base_port_ << ", shift=" << shift << std::endl;
    throw InvalidPortRequested(ostr);
  }
  return port;
}

std::string
ApachePorts::get_port_string(std::size_t shift)
  /*throw (InvalidPortRequested)*/
{
  char buf[6];
  std::snprintf(buf, 6, "%d", get_port(shift));
  return buf;
}
