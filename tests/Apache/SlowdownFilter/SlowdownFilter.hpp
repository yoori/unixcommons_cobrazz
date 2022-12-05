/**
 * @author Pavel Gubin <pgubin@ipmce.ru>
 */
#ifndef APACHE_SLOWDOWN_FILTER_HPP
#define APACHE_SLOWDOWN_FILTER_HPP

#include <time.h>

#include <ReferenceCounting/ReferenceCounting.hpp>

#include <Apache/Module.hpp>


class SlowdownFilterModule :
  public Apache::ConfigParser,
  public Apache::InsertFilterHook<SlowdownFilterModule>,
  public ReferenceCounting::AtomicImpl
{
public:
  typedef ReferenceCounting::QualPtr<SlowdownFilterModule>
    SlowdownFilterModule_var;
  static SlowdownFilterModule_var instance;

  class SlowdownFilter : public Apache::RequestOutputFilter
  {
  public:
    SlowdownFilter(request_rec* r, timespec& delay) throw ();
    virtual apr_status_t
    filter(ap_filter_t* f, apr_bucket_brigade* bb) throw ();

  private:
    timespec delay_;
  };

public:
  SlowdownFilterModule() throw ();

  virtual const char*
  handle_command(const ConfigArgs& args) throw ();
  virtual void
  insert_filter(request_rec* r) throw ();

protected:
  virtual
  ~SlowdownFilterModule() throw ();

private:
  timespec delay_;
};

#endif
