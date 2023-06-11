#include "UServerUtils/MetricsHTTPProvider.hpp"
#include "Generics/CompositeMetricsProvider.hpp"
#include "Generics/MetricsProvider.hpp"
#include <unistd.h>
//using UServerUtils;

int main(int /*argc*/, char** /*argv*/)
{
  ReferenceCounting::SmartPtr<Generics::CompositeMetricsProvider> cmp(new Generics::CompositeMetricsProvider);
  ReferenceCounting::SmartPtr<UServerUtils::MetricsHTTPProvider> m(new UServerUtils::MetricsHTTPProvider(cmp,8081,"/metrics"));

  m->activate_object();
  for(long i=0; i<100; i++)
  {
      std::map<std::string,std::string> m;
      m["method"]="POST";
      m["url"]="/messages";
      cmp->set_value_prometheus("http_requests_total",m, i);
      sleep(1);
  }
  m->deactivate_object();
  m->wait_object();

  return 0;
}
