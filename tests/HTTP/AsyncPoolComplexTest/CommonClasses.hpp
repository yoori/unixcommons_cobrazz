#pragma once

#include <HTTP/HttpTestCommons/CommonClasses.hpp>

//
// class CallBackProxy
//

class CallBackProxy : public HTTP::ResponseCallback,
  public ReferenceCounting::AtomicImpl
{
public:
  CallBackProxy(Sync::Semaphore& finish_semaphore,
    HTTP::ResponseCallback *p_impl) /*throw(eh::Exception)*/;
  virtual
  ~CallBackProxy() noexcept;
    /**
     * Called when request succeeded
     * @param data response
     */
    virtual void
    on_response(const HTTP::ResponseInformation& data) noexcept;

    /**
     * Called when request succeeded and it is not possible to call on_response
     * Should return control ASAP
     * @param data response
     */
    virtual void
    quick_on_response(const HTTP::ResponseInformation& data) noexcept;

    /**
     * Called when request failed
     * @param description error message
     * @param data request
     */
    virtual void
    on_error(const String::SubString& description, const HTTP::RequestInformation& data)
      noexcept;

    /**
     * Called when request failed and it is not possible to call on_error
     * Should return control ASAP
     * @param description error message
     * @param data request
     */
    virtual void
    quick_on_error(const String::SubString& description,
      const HTTP::RequestInformation& data)
      noexcept;

private:
  HTTP::ResponseCallback_var p_impl_;
  Sync::Semaphore& finish_semaphore_;
};

//
// class CheckUpCallback
//

class CheckUpCallback : public SimpleCounterCallback
{
public:

  CheckUpCallback(HTTP::PoolPolicy* policy, const std::string& get_str,
    const std::string& post_str, const std::string& pattern_beg,
    const std::string& pattern_end);

  virtual void
  on_response(const HTTP::ResponseInformation& data) noexcept;

  virtual void
  on_error(const String::SubString& descr,
    const HTTP::RequestInformation& data) noexcept;

  virtual void
  print_stat(std::ostream& ostr) /*throw (eh::Exception)*/;

  const TestCommons::Counter&
  get_checkup_counter() const noexcept;

protected:

  virtual
  ~CheckUpCallback() noexcept;

private:
  const std::string GET_STR_;
  const std::string POST_STR_;
  const std::string PATTERN_BEG_;
  const std::string PATTERN_END_;

  TestCommons::Counter response_checkup_;
};

typedef ReferenceCounting::QualPtr<CheckUpCallback> CheckUpCallback_var;

//
// class CTTestInterface
//

class CTTestInterface: public TestInterface
{
public:

  CTTestInterface(HTTP::HttpInterface* pool, unsigned int test_duration,
    unsigned int making_requests_duration, unsigned int tasks_per_test,
    unsigned int functors_per_task) /*throw (eh::Exception)*/;

  virtual const std::string
  additional_http_query() /*throw (eh::Exception)*/;

  virtual void
  execute() noexcept;

  bool
  is_error(const char* test_name, const TestCommons::Counter* add_counter,
    const TestCommons::Counter* callb_counter,
    const TestCommons::Counter* checkup_counter)
    /*throw (eh::Exception)*/;

  virtual std::string
  checkup_and_print_stat() /*throw (eh::Exception)*/ = 0;

protected:

  virtual
  ~CTTestInterface() noexcept;

  HTTP::HttpInterface_var pool_;
  std::ostringstream stat_;
  std::unique_ptr<Requester> requester_;
  unsigned int run_period_;
  unsigned int tasks_count_;
  unsigned int functors_count_;
};

typedef ReferenceCounting::QualPtr<CTTestInterface> CTTestInterface_var;
