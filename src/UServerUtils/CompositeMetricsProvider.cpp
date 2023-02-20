#include "CompositeMetricsProvider.hpp"

class to_string_visitor : public boost::static_visitor<>
{
public:
    std::string str;
    void operator()(long i)
    {
        str=std::to_string(i);
    }
    void operator()(double i)
    {
        str=std::to_string(i);
    }
    void operator()(const std::string i)
    {
        str=i;
    }

};


void CompositeMetricsProvider::add_provider(MetricsProvider* p)
{
    providers.insert(p);
}


MetricsProvider::MetricArray CompositeMetricsProvider::get_values()
{
    std::lock_guard<std::mutex> g(mx);
    MetricArray ret;
    for(auto& z: container)
    {
        ret.push_back(z);
    }
    for(auto& z:providers)
    {
      auto arr=z->get_values();
      for(auto &x: arr)
      {
          ret.push_back(x);
      }
    }
    return ret;

}

std::map<std::string,std::string> CompositeMetricsProvider::getStringValues()
{
    std::lock_guard<std::mutex> g(mx);
    std::map<std::string,std::string> ret;
    MetricArray arr=get_values();
    for(auto& z:arr)
    {
          auto& key=z.first;
          auto v=z.second;
          to_string_visitor vis;
          boost::apply_visitor(vis,v);
          ret[key]=vis.str;
    }
    return ret;
}
