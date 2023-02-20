#include <UServerUtils/MetricsHTTPProvider.hpp>
#include <unistd.h>

int main(int argc, char* argv[])
{
  UServerUtils::MetricsHTTPProvider_var m = new UServerUtils::MetricsHTTPProvider(8081,"/sample/data");
  m->activate_object();
  for(int i=0;i<100;i++)
  {
      m->add_value("key-"+std::to_string(i),i);
      m->add_value("key2-"+std::to_string(i),i+100);
      sleep(1);
  }
  m->deactivate_object();
  m->wait_object();
  return 0;
}
