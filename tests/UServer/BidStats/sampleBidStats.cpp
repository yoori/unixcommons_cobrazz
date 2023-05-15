#include "UServerUtils/BidStatisticsPrometheus.hpp"
#include "Generics/CompositeMetricsProvider.hpp"
#include "Generics/MetricsProvider.hpp"
#include "UServerUtils/metrics_raii.hpp"
#include <unistd.h>
int main(int /*argc*/, char** /*argv*/)
{

    CompositeMetricsProvider *cmp=new CompositeMetricsProvider;

    UServerUtils::MetricsHTTPProvider *m=new UServerUtils::MetricsHTTPProvider(cmp,8081,"/metrics");
    m->activate_object();
    for(long i=0; i<1000; i++)
    {
        BidStatisticsPrometheusInc(cmp,rand()%%10);
        sleep(1);

    }
    m->deactivate_object();
    m->wait_object();
    delete m;
    return 0;
}
