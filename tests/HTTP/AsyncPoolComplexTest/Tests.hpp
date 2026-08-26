#pragma once

#include "CommonClasses.hpp"

//
// class EchoTest
//

class EchoTest : public CTTestInterface
{
public:

  static const char* usage() noexcept;

  EchoTest(Sync::Semaphore& finish_semaphore,
           HTTP::HttpInterface* pool, unsigned int test_duration,
           unsigned int making_requests_duration, unsigned int tasks_per_test,
           unsigned int functors_per_task, bool log_needed = false)
    /*throw (eh::Exception)*/;

  virtual std::string
  checkup_and_print_stat() /*throw (eh::Exception)*/;

protected:

  virtual
  ~EchoTest() noexcept;

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

  static const char* usage() noexcept;

  NonExistanceTest(Sync::Semaphore& finish_semaphore,
      HTTP::HttpInterface* pool, unsigned int test_duration,
      unsigned int making_requests_duration, unsigned int tasks_per_test,
      unsigned int functors_per_task, bool log_needed = false)
    /*throw (eh::Exception)*/;

  virtual std::string
  checkup_and_print_stat() /*throw (eh::Exception)*/;

protected:

  virtual
  ~NonExistanceTest() noexcept;

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

  static const char* usage() noexcept;

  BadAddressTest(Sync::Semaphore& finish_semaphore,
      HTTP::HttpInterface* pool, unsigned int test_duration,
      unsigned int making_requests_duration, unsigned int tasks_per_test,
      unsigned int functors_per_task, bool log_needed = false)
    /*throw (eh::Exception)*/;

  virtual std::string
  checkup_and_print_stat() /*throw (eh::Exception)*/;

protected:

  virtual
  ~BadAddressTest() noexcept;

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
  on_response(const HTTP::ResponseInformation& data) noexcept;

  virtual void
  on_error(const String::SubString& description,
    const HTTP::RequestInformation& data) noexcept;

  void check() noexcept;

protected:
  virtual
  ~InterruptCallback() noexcept;

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

  static const char* usage() noexcept;

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
  ~InterruptTest() noexcept;

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

  static const char* usage() noexcept;

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
  ~BadRespTest() noexcept;

private:

  SimpleCounterCallback_var my_cb_;
  Sync::PosixMutex mutex_;
  int counter_;
  bool log_needed_;
};
