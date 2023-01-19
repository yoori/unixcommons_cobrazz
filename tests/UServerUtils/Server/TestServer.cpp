/**
 * @file TestServer.cpp
 */

#include <iostream>
#include <UServerUtils/Server.hpp>


int
main() throw ()
{
  try
  {
    std::cout << "UServer simple test" << std::endl;
    UServerUtils::Server* server = new UServerUtils::Server();
    std::cout << "add_grpc_server_component()" << std::endl;
    server->add_grpc_server(20000);
    std::cout << "activate_object()" << std::endl;
    server->activate_object();
    std::cout << "active()" << std::endl;
    server->active();
    std::cout << "deactivate_object()" << std::endl;
    server->deactivate_object();
    std::cout << "wait_object()" << std::endl;
    server->wait_object();
    std::cout << "remove_ref()" << std::endl;
    server->remove_ref();
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

