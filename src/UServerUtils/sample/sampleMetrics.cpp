#include "MetricsHTTPProvider.hpp"
#include "CompositeMetricsProvider.hpp"
#include "MetricsProvider.hpp"
#include <unistd.h>
//using UServerUtils;
int main(int argc, char* argv[])
{

    CompositeMetricsProvider *cmp=new CompositeMetricsProvider;

    UServerUtils::MetricsHTTPProvider *m=new UServerUtils::MetricsHTTPProvider(cmp,8081,"/sample/data");
    m->activate_object();
    for(long i=0;i<100;i++)
    {
        cmp->add_value(("key-"+std::to_string(i)).c_str(),i);
        cmp->add_value(("key2-"+std::to_string(i)).c_str(),i+100);
        sleep(1);
    }
    m->deactivate_object();
    m->wait_object();
    delete m;
    return 0;
}
