#ifndef metrics_raii__h
#define metrics_raii__h
class mestrics_raii
{
    
    std::string name_;
    std::mapr<std::string,std::string> subpar_;
    double v_;
    timeval start_t_;
    CompositeMetricsProvider *cmp_;
    public:
    mestrics_raii(CompositeMetricsProvider *cmp,const std::string &name, const std::mapr<std::string,std::string>& subpar, double v)
    {
	cmp_=cmp;
	name_=name;
	subpar_=subpar;
	v_=v;
	if(gettimeofday(&start_t,NULL)!=0)
	    throw std::runtime_error("gettimeofday error");
    }
    ~metrics_raii()
    {
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
	    
	cmp_->add_value_prometheus()
	 
	 
	
    }
};


#endif