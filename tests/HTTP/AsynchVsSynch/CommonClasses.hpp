#ifndef _ASYNCH_VS_SYNCH_TEST_COMMON_CLASSES_HPP_
#define _ASYNCH_VS_SYNCH_TEST_COMMON_CLASSES_HPP_

#include <HTTP/HttpTestCommons/CommonClasses.hpp>
#include <vector>
#include <string>

//
// class NotificationCallback
//

class NotificationCallback : public SimpleCounterCallback
{
public:

  NotificationCallback(HTTP::PoolPolicy_var policy, unsigned int notify_number)
    /*throw(eh::Exception)*/;

  virtual void
  on_response(const HTTP::ResponseInformation& data) throw ();

  virtual void
  on_error(const String::SubString& descr,
    const HTTP::RequestInformation& data) throw ();

  Sync::Semaphore&
  get_semaphore() throw();

protected:

  virtual
  ~NotificationCallback() throw ();

private:

  void check() throw();

  std::unique_ptr<Sync::Semaphore> sem_;
  volatile _Atomic_word notify_number_;
  std::atomic<int> waits_number_;
};

typedef ReferenceCounting::QualPtr<NotificationCallback> NotificationCallback_var;

//
// class VSTestInterface
//

class VSTestInterface: public TestInterface
{
public:

  VSTestInterface(Sync::Semaphore& finish_sem) /*throw(eh::Exception)*/;
  
  virtual void print_stat(std::ostream& out) const /*throw(eh::Exception)*/ = 0;

protected:

  virtual
  ~VSTestInterface() throw ();
  
  Sync::Semaphore& finish_sem_;
};

typedef ReferenceCounting::QualPtr<VSTestInterface> VSTestInterface_var;

//
// struct InfoToCallback
//

struct InfoToCallback
{
  HTTP::PoolPolicy_var policy;
  size_t type;
  const std::vector</*const */std::string>& requests_by_type;
  Sync::Semaphore& threads_sem;
  Sync::Semaphore& main_sem;
  
  InfoToCallback(HTTP::PoolPolicy_var new_policy, size_t new_type,
      const std::vector</*const */std::string>& new_requests,
      Sync::Semaphore& thrs_sem, Sync::Semaphore& sem)
    /*throw(eh::Exception)*/;
};

#endif
