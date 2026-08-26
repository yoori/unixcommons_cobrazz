#pragma once

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
    SlowdownFilter(request_rec* r, timespec& delay) noexcept;
    virtual apr_status_t
    filter(ap_filter_t* f, apr_bucket_brigade* bb) noexcept;

  private:
    timespec delay_;
  };

public:
  SlowdownFilterModule() noexcept;

  virtual const char*
  handle_command(const ConfigArgs& args) noexcept;
  virtual void
  insert_filter(request_rec* r) noexcept;

protected:
  virtual
  ~SlowdownFilterModule() noexcept;

private:
  timespec delay_;
};
