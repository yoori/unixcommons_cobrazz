#ifndef metrics_raii__h
#define metrics_raii__h

/// RAII objecft
/// save start time in constructor and appdend metrics on destructor
/// see example in tests/UServer/MetricsRaii/sampleMetricsRAII.cpp
class metrics_raii
{

    std::string name_;
    std::map<std::string,std::string> subpar_;
    timeval start_t_;
    Generics::CompositeMetricsProvider *cmp_;
public:
    metrics_raii(Generics::CompositeMetricsProvider *cmp,const std::string &name, const std::map<std::string,std::string>& subpar)
    {
        cmp_=cmp;
        name_=name;
        subpar_=subpar;
        if(gettimeofday(&start_t_,NULL)!=0)
            throw std::runtime_error("gettimeofday error");
    }
    ~metrics_raii()
    {
        try {
            timeval end_t;
            if(gettimeofday(&end_t,NULL)!=0)
                throw std::runtime_error("gettimeofday error");
            int microsec=(end_t.tv_sec-start_t_.tv_sec)*1000000 + end_t.tv_usec-start_t_.tv_usec;
            int millisec=microsec/1000;
            std::string name2=name_;
            if(millisec<5)
                name2+="_5ms";
            else if(millisec<10)
                name2+="_10ms";
            else if(millisec<20)
                name2+="_20ms";
            else if(millisec<50)
                name2+="_50ms";
            else if(millisec<100)
                name2+="_100ms";
            else
                name2+="_gt_100ms";

            cmp_->add_value_prometheus(name2,subpar_,1);
        }
        catch(std::exception &e)
        {
            fprintf(stderr, "std::exception %s",e.what());
        }
        catch(...)
        {
            fprintf(stderr, "exception unknown ... ");
        }




    }
};


#endif