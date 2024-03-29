#ifndef _COMPLEX_TEST_TESTS_HPP_
#define _COMPLEX_TEST_TESTS_HPP_

#include "CommonClasses.hpp"

//
// class EchoTest
//

class EchoTest : public CTTestInterface
{
public:

  static const char* usage() throw();

  EchoTest(Sync::Semaphore& finish_semaphore,
           HTTP::HttpInterface* pool, unsigned int test_duration,
           unsigned int making_requests_duration, unsigned int tasks_per_test,
           unsigned int functors_per_task, bool log_needed = false)
    /*throw (eh::Exception)*/;

  virtual std::string
  checkup_and_print_stat() /*throw (eh::Exception)*/;

protected:

  virtual
  ~EchoTest() throw ();

private:
  CheckUpCallback_var my_cb_;
  bool log_needed_;
};

//
// class NonExistanceTest
//

class NonExistanceTest : public CTTestInterface
{
public:

  static const char* usage() throw();

  NonExistanceTest(Sync::Semaphore& finish_semaphore,
      HTTP::HttpInterface* pool, unsigned int test_duration,
      unsigned int making_requests_duration, unsigned int tasks_per_test,
      unsigned int functors_per_task, bool log_needed = false)
    /*throw (eh::Exception)*/;

  virtual std::string
  checkup_and_print_stat() /*throw (eh::Exception)*/;

protected:

  virtual
  ~NonExistanceTest() throw ();

private:
  SimpleCounterCallback_var my_cb_;
  bool log_needed_;
};

//
// class BadAddressTest
//

class BadAddressTest : public CTTestInterface
{
public:

  static const char* usage() throw();

  BadAddressTest(Sync::Semaphore& finish_semaphore,
      HTTP::HttpInterface* pool, unsigned int test_duration,
      unsigned int making_requests_duration, unsigned int tasks_per_test,
      unsigned int functors_per_task, bool log_needed = false)
    /*throw (eh::Exception)*/;

  virtual std::string
  checkup_and_print_stat() /*throw (eh::Exception)*/;

protected:

  virtual
  ~BadAddressTest() throw ();

private:

  SimpleCounterCallback_var my_cb_;
  bool log_needed_;
};

//
// class InterruptCallback
//

class InterruptCallback: public SimpleCounterCallback
{
public:
  InterruptCallback(HTTP::PoolPolicy* policy, Sync::Semaphore& sem)
    /*throw(eh::Exception)*/;

  virtual void
  on_response(const HTTP::ResponseInformation& data) throw ();

  virtual void
  on_error(const String::SubString& description,
    const HTTP::RequestInformation& data) throw ();

  void check() throw();

protected:
  virtual
  ~InterruptCallback() throw ();

private:
  Sync::Semaphore& sem_;
  std::atomic<int> cnt_;
};

//
// class InterruptTest
//

class InterruptTest : public CTTestInterface
{
public:

  static const char* usage() throw();

  InterruptTest(Sync::Semaphore& finish_semaphore,
                HTTP::HttpInterface* pool, unsigned int test_duration,
                unsigned int making_requests_duration, unsigned int tasks_per_test,
                unsigned int functors_per_task, bool log_needed = false)
    /*throw (eh::Exception)*/;

  virtual const std::string
  additional_http_query() /*throw (eh::Exception)*/;

  virtual std::string
  checkup_and_print_stat() /*throw (eh::Exception)*/;

protected:

  virtual
  ~InterruptTest() throw ();

private:

  SimpleCounterCallback_var my_cb_;
  Sync::PosixMutex mutex_;
  int counter_;
  bool log_needed_;
  Sync::Semaphore sem_;
  std::string tmp_dir;
};

//
// class BadRespTest
//

class BadRespTest : public CTTestInterface
{
public:

  static const char* usage() throw();

  BadRespTest(Sync::Semaphore& finish_semaphore,
              HTTP::HttpInterface* pool, unsigned int test_duration,
              unsigned int making_requests_duration, unsigned int tasks_per_test,
              unsigned int functors_per_task, bool log_needed = false)
    /*throw (eh::Exception)*/;

  virtual const std::string
  additional_http_query() /*throw (eh::Exception)*/;

  virtual std::string
  checkup_and_print_stat() /*throw (eh::Exception)*/;

protected:

  virtual
  ~BadRespTest() throw ();

private:

  SimpleCounterCallback_var my_cb_;
  Sync::PosixMutex mutex_;
  int counter_;
  bool log_needed_;
};

#endif
