#ifndef ___CompositeMetricsProvider__H
#define ___CompositeMetricsProvider__H
#include <map>
#include <set>
#include <mutex>
#include "MetricsProvider.hpp"
#include "ReferenceCounting/SmartPtr.hpp"

namespace Generics
{
    class CompositeMetricsProvider : public MetricsProvider
    {
        std::map<std::string,Value> container;
        std::set<ReferenceCounting::SmartPtr<MetricsProvider> > providers;
        /// prometheus
        std::map<std::string, std::map<std::map<std::string,std::string>, double> > prometheus_container;

        std::mutex mx;
    public:
        void
        add_provider(MetricsProvider* p);


        MetricArray get_values();
        std::map<std::string, std::map<std::map<std::string,std::string>, double> > get_prometheus_values()
        {
            std::lock_guard<std::mutex> g(mx);
            return prometheus_container;

        }
        std::string get_prometheus_formatted()
        {
            std::string out;
            for(auto &a: prometheus_container)
            {
        	auto & name=a.first;
                for(auto &b: a.second)
                {
                    auto& maddons=b.first;
                    auto& value=b.second;
                    std::string addons;
                    int idx=0;
                    for(auto &d: maddons)
                    {
                        if(idx!=0)
                            addons+=", ";
                        addons=d.first+"=\""+d.second+"\"";
                    }
                    out+=name+"{"+addons+"} "+std::to_string(value)+"\n";

                }
            }
            return out;
        }
        std::map<std::string,std::string> getStringValues();

        void add_value(std::string_view n,double v)
        {
            std::lock_guard<std::mutex> g(mx);
            container[std::string(n)]=v;
        }
        void add_value(std::string_view n,long v)
        {
            std::lock_guard<std::mutex> g(mx);
            container[std::string(n)]=v;
        }
        void add_value(std::string_view n,const std::string& v)
        {
            std::lock_guard<std::mutex> g(mx);
            container[std::string(n)]=v;
        }
        void set_value_prometheus(const std::string & parameter, const std::map<std::string, std::string>& par2, double value)
        {
            std::lock_guard<std::mutex> g(mx);
            prometheus_container[parameter][par2]=value;
        }
        void add_value_prometheus(const std::string & parameter, const std::map<std::string, std::string>& par2, double value)
        {
            std::lock_guard<std::mutex> g(mx);
            prometheus_container[parameter][par2]+=value;
        }
    };
    typedef ReferenceCounting::SmartPtr<CompositeMetricsProvider> CompositeMetricsProvider_var;

}
#endif

