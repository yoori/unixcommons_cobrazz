#include <unistd.h>

#include "UServerUtils/MetricsHTTPProvider.hpp"
#include "Generics/CompositeMetricsProvider.hpp"
#include "Generics/MetricsProvider.hpp"

int main(int /*argc*/, char** /*argv*/)
{
  ReferenceCounting::SmartPtr<Generics::CompositeMetricsProvider> cmp(new Generics::CompositeMetricsProvider);
  ReferenceCounting::SmartPtr<UServerUtils::MetricsHTTPProvider> m(new UServerUtils::MetricsHTTPProvider(cmp,8081,"/metrics"));

  m->activate_object();
  for(long i=0; i<100; i++)
  {
      cmp->add_value(("key-"+std::to_string(i)).c_str(),i);
      cmp->add_value(("key2-"+std::to_string(i)).c_str(),i+100);
      sleep(1);
  }
  m->deactivate_object();
  m->wait_object();

  return 0;
}
