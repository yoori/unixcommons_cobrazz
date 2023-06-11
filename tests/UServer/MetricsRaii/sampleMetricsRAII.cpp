#include <unistd.h>
#include <map>

#include "UServerUtils/MetricsHTTPProvider.hpp"
#include "Generics/CompositeMetricsProvider.hpp"
#include "Generics/MetricsProvider.hpp"
#include "UServerUtils/metrics_raii.hpp"

int main(int /*argc*/, char** /*argv*/)
{
  ReferenceCounting::SmartPtr<Generics::CompositeMetricsProvider> cmp(new Generics::CompositeMetricsProvider);
  ReferenceCounting::SmartPtr<UServerUtils::MetricsHTTPProvider> m(
    new UServerUtils::MetricsHTTPProvider(cmp,8081,"/metrics"));

  m->activate_object();
  for(long i=0; i<1000; i++)
  {
      {
          std::map<std::string,std::string> m;
          m["user_bind_mapper"]="get_user_id";
          metrics_raii __r(cmp,"user_bind_resolve_calls", m);
          usleep(rand()%1000 * 1000); /// request imitation
      }
      {
          std::map<std::string,std::string> m;
          m["user_bind_mapper"]="get_user_id";
          metrics_raii __r(cmp,"user_bind_add_calls", m);
          usleep(rand()%1000 * 1000); /// request imitation
      }

  }
  m->deactivate_object();
  m->wait_object();

  return 0;
}
