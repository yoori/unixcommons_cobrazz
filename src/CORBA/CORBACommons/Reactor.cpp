#include <sys/poll.h>

#include <list>

#include <ace/TP_Reactor.h>

#include <Sync/Semaphore.hpp>

#include <Generics/Proc.hpp>
#include <Generics/BitAlgs.hpp>
#include <Generics/Descriptors.hpp>
#include <Generics/TAlloc.hpp>

#include "CorbaAdaptersInternal.hpp"


namespace
{
  union FDSet
  {
  public:
    class Find
    {
    public:
      Find() noexcept;
      int
      descriptor() noexcept;

    private:
      int elem_, bit_;

      friend union FDSet;
    };

    FDSet(bool init = true) noexcept;
    FDSet& operator=(const FDSet&) = default;

    fd_set*
    operator &() noexcept;

    void
    find_first(Find& f) noexcept;

    void
    find_next(Find& f) noexcept;

  private:
    void
    find_from_(Find& f, unsigned from) noexcept;

    enum { SIZE = CORBACommons::DESCRIPTORS / 64 };
    uint64_t bits_[SIZE];
    fd_set set_;
  };


  FDSet::Find::Find() noexcept
    : elem_(0), bit_(-1)
  {
  }

  int
  FDSet::Find::descriptor() noexcept
  {
    return elem_ * 64 + bit_;
  }


  FDSet::FDSet(bool init) noexcept
  {
    if (init)
    {
      ::memset(bits_, 0, sizeof(bits_));
    }
  }

  fd_set*
  FDSet::operator &() noexcept
  {
    return &set_;
  }

  void
  FDSet::find_first(Find& f) noexcept
  {
    find_from_(f, 0);
  }

  void
  FDSet::find_next(Find& f) noexcept
  {
    if (f.descriptor() < 0)
    {
      return;
    }
    if (f.bit_ != 63)
    {
      unsigned res = Generics::BitAlgs::lowest_bit_64(bits_[f.elem_] &
        (~static_cast<uint64_t>(0) << (f.bit_ + 1)));
      if (res != 64)
      {
        f.bit_ = res;
        return;
      }
    }
    find_from_(f, f.elem_ + 1);
  }

  void
  FDSet::find_from_(Find& f, unsigned from) noexcept
  {
    for (unsigned i = from; i < SIZE; i++)
    {
      if (bits_[i])
      {
        f.elem_ = i;
        f.bit_ = Generics::BitAlgs::lowest_bit_64(bits_[i]);
        return;
      }
    }
    f = Find();
  }
}

namespace
{
  class Reactor : public ACE_Reactor_Impl
  {
  public:
    explicit
    Reactor(ACE_Timer_Queue* tq) noexcept;
    virtual
    ~Reactor() noexcept;

    virtual
    int
    open(size_t, bool = false, ACE_Sig_Handler* = 0, ACE_Timer_Queue* = 0,
      int = 0, ACE_Reactor_Notify* = 0) noexcept;
    virtual
    int
    current_info(ACE_HANDLE, size_t&) noexcept;
    virtual
    int
    set_sig_handler(ACE_Sig_Handler*) noexcept;
    virtual
    int
    timer_queue(ACE_Timer_Queue *) noexcept;
    virtual
    ACE_Timer_Queue*
    timer_queue() const noexcept;
    virtual
    int
    close() noexcept;
    virtual
    int
    work_pending(const ACE_Time_Value& = ACE_Time_Value::zero) noexcept;
    virtual
    int
    handle_events(ACE_Time_Value* max_wait_time = 0) noexcept;
    virtual
    int
    alertable_handle_events(ACE_Time_Value* = 0) noexcept;
    virtual
    int
    handle_events(ACE_Time_Value&) noexcept;
    virtual
    int
    alertable_handle_events(ACE_Time_Value&) noexcept;
    virtual
    int
    deactivated() noexcept;
    virtual
    void
    deactivate(int do_stop) noexcept;
    virtual
    int
    register_handler(ACE_Event_Handler* event_handler,
      ACE_Reactor_Mask mask) noexcept;
    virtual
    int
    register_handler(ACE_HANDLE, ACE_Event_Handler*, ACE_Reactor_Mask)
      noexcept;
    virtual
    int
    register_handler(ACE_HANDLE, ACE_HANDLE, ACE_Event_Handler*,
      ACE_Reactor_Mask) noexcept;
    virtual
    int
    register_handler(const ACE_Handle_Set&, ACE_Event_Handler*,
      ACE_Reactor_Mask) noexcept;
    virtual
    int
    register_handler(int, ACE_Event_Handler*, ACE_Sig_Action*,
      ACE_Event_Handler** = 0, ACE_Sig_Action* = 0) noexcept;
    virtual
    int
    register_handler(const ACE_Sig_Set&, ACE_Event_Handler*,
      ACE_Sig_Action* = 0) noexcept;
    virtual
    int
    remove_handler(ACE_Event_Handler*, ACE_Reactor_Mask) noexcept;
    virtual
    int
    remove_handler(ACE_HANDLE handle, ACE_Reactor_Mask mask) noexcept;
    virtual
    int
    remove_handler(const ACE_Handle_Set&, ACE_Reactor_Mask) noexcept;
    virtual
    int
    remove_handler(int, ACE_Sig_Action*, ACE_Sig_Action* = 0, int = -1)
      noexcept;
    virtual
    int
    remove_handler(const ACE_Sig_Set&) noexcept;
    virtual
    int
    suspend_handler(ACE_Event_Handler*) noexcept;
    virtual
    int
    suspend_handler(ACE_HANDLE) noexcept;
    virtual
    int
    suspend_handler(const ACE_Handle_Set&) noexcept;
    virtual
    int
    suspend_handlers() noexcept;
    virtual
    int
    resume_handler(ACE_Event_Handler*) noexcept;
    virtual
    int
    resume_handler(ACE_HANDLE handle) noexcept;
    virtual
    int
    resume_handler(const ACE_Handle_Set&) noexcept;
    virtual
    int
    resume_handlers() noexcept;
    virtual
    int
    resumable_handler() noexcept;
    virtual
    bool
    uses_event_associations() noexcept;
    virtual
    long
    schedule_timer(ACE_Event_Handler*, const void*,
      const ACE_Time_Value&, const ACE_Time_Value& = ACE_Time_Value::zero)
      noexcept;
    virtual
    int
    reset_timer_interval(long, const ACE_Time_Value&) noexcept;
    virtual
    int
    cancel_timer(ACE_Event_Handler*, int = 1) noexcept;
    virtual
    int
    cancel_timer(long, const void** = 0, int = 1) noexcept;
    virtual
    int
    schedule_wakeup(ACE_Event_Handler*, ACE_Reactor_Mask) noexcept;
    virtual
    int
    schedule_wakeup(ACE_HANDLE, ACE_Reactor_Mask) noexcept;
    virtual
    int
    cancel_wakeup(ACE_Event_Handler*, ACE_Reactor_Mask) noexcept;
    virtual
    int
    cancel_wakeup(ACE_HANDLE, ACE_Reactor_Mask) noexcept;
    virtual
    int
    notify(ACE_Event_Handler* event_handler = 0,
      ACE_Reactor_Mask mask = ACE_Event_Handler::EXCEPT_MASK,
      ACE_Time_Value* max_wait_time = 0) noexcept;
    virtual
    void
    max_notify_iterations(int) noexcept;
    virtual
    int
    max_notify_iterations() noexcept;
    virtual
    int
    purge_pending_notifications(ACE_Event_Handler* = 0,
      ACE_Reactor_Mask = ACE_Event_Handler::ALL_EVENTS_MASK) noexcept;
    virtual
    ACE_Event_Handler*
    find_handler(ACE_HANDLE) noexcept;
    virtual
    int
    handler(ACE_HANDLE, ACE_Reactor_Mask, ACE_Event_Handler** = 0) noexcept;
    virtual
    int
    handler(int, ACE_Event_Handler** = 0) noexcept;
    virtual
    bool
    initialized() noexcept;
    virtual
    size_t
    size() const noexcept;
    virtual
    ACE_Lock&
    lock() noexcept;
    virtual
    void
    wakeup_all_threads() noexcept;
    virtual
    int
    owner(ACE_thread_t, ACE_thread_t* = 0) noexcept;
    virtual
    int
    owner(ACE_thread_t*) noexcept;
    virtual
    bool
    restart() noexcept;
    virtual
    bool
    restart(bool) noexcept;
    virtual
    void
    requeue_position(int) noexcept;
    virtual
    int
    requeue_position() noexcept;
    virtual
    int
    mask_ops(ACE_Event_Handler*, ACE_Reactor_Mask, int) noexcept;
    virtual
    int
    mask_ops(ACE_HANDLE, ACE_Reactor_Mask, int) noexcept;
    virtual
    int
    ready_ops(ACE_Event_Handler*, ACE_Reactor_Mask, int) noexcept;
    virtual
    int
    ready_ops(ACE_HANDLE, ACE_Reactor_Mask, int) noexcept;
    virtual
    void
    dump() const noexcept;

  private:
    typedef std::map<int, ACE_Event_Handler*, std::less<int>,
      Generics::TAlloc::Aggregated<std::map<int, ACE_Event_Handler*>::value_type,
        CORBACommons::DESCRIPTORS / CORBACommons::PARTS>> Handlers;

    /*
    struct Handlers: protected std::vector<std::pair<int, ACE_Event_Handler*> >
    {
      typedef std::vector<int, std::pair<ACE_Event_Handler*, bool> >::iterator iterator;

      Handlers()
      {
        resize(CORBACommons::DESCRIPTORS, std::make_pair(0, false));
      }

    protected:
      std::vector<bool> actual_;
    };
    */

    typedef std::list<ACE_Event_Handler*,
      Generics::TAlloc::Aggregated<ACE_Event_Handler*, CORBACommons::DESCRIPTORS>> Next;

    struct Part
    {
      Part() noexcept;

      Sync::PosixMutex select;

      Sync::PosixMutex data;
      FDSet wait;
      Handlers handlers;
      bool in_select;

      Generics::NonBlockingReadPipe pipe;
    };

    Part parts_[CORBACommons::PARTS];

    Sync::PosixMutex queue_;
    Sync::Semaphore sem_;
    Next next_;

    volatile sig_atomic_t exit_;
    volatile _Atomic_word waiters_;

    enum
    {
      PARTS_MASK = CORBACommons::PARTS - 1
    };

    static
    unsigned
    part_(unsigned fd) noexcept;

    static
    unsigned
    adapt_fd_for_fdset_(unsigned fd) noexcept;
  };


  Reactor::Part::Part() noexcept
    : in_select(false)
  {
    FD_SET(pipe.read_descriptor(), &wait);
  }


  Reactor::Reactor(ACE_Timer_Queue* tq) noexcept
    : sem_(0), exit_(false), waiters_(0)
  {
    delete tq;
  }

  Reactor::~Reactor() noexcept
  {
  }

  unsigned
  Reactor::part_(unsigned fd) noexcept
  {
    return fd & PARTS_MASK;
    //return (fd >> 6) & PARTS_MASK;
  }

  unsigned
  Reactor::adapt_fd_for_fdset_(unsigned fd) noexcept
  {
    return fd; // % (16 * 1024);
  }

  int
  Reactor::open(size_t, bool, ACE_Sig_Handler*, ACE_Timer_Queue*, int,
    ACE_Reactor_Notify*) noexcept
  {
    abort();
    return 0;
  }

  int
  Reactor::current_info(ACE_HANDLE, size_t&) noexcept
  {
    abort();
  }

  int
  Reactor::set_sig_handler(ACE_Sig_Handler*) noexcept
  {
    abort();
  }

  int
  Reactor::timer_queue(ACE_Timer_Queue *) noexcept
  {
    abort();
  }

  ACE_Timer_Queue*
  Reactor::timer_queue() const noexcept
  {
    return 0;
  }

  int
  Reactor::close() noexcept
  {
    return 0;
  }

  int
  Reactor::work_pending(const ACE_Time_Value&) noexcept
  {
    abort();
    return 0;
  }

  int
  Reactor::handle_events(ACE_Time_Value* max_wait_time) noexcept
  {
    (void)max_wait_time;
    assert(!max_wait_time);
    for (unsigned p = 0; p < CORBACommons::PARTS; p++)
    {
      Part& part = parts_[p];
      if (Sync::PosixTryGuard guard{part.select})
      {
        while (!exit_)
        {
          FDSet ready(ready);

          {
            Sync::PosixGuard guard(part.data);
            ready = part.wait;
            part.in_select = true;
          }

          int count = select(CORBACommons::DESCRIPTORS, &ready, 0, 0, 0);
          if (count <= 0)
          {
            continue;
          }

          if (FD_ISSET(part.pipe.read_descriptor(), &ready))
          {
            FD_CLR(part.pipe.read_descriptor(), &ready);
            count--;
            char buf[4096];
            part.pipe.read(buf, sizeof(buf));
          }

          if (count)
          {
            FDSet::Find find;
            {
              Sync::PosixGuard guard(part.data);
              part.in_select = false;
              for (ready.find_first(find); count--; ready.find_next(find))
              {
                int fd = find.descriptor();
                assert(fd >= 0);
                FD_CLR(fd, &part.wait);
                Handlers::iterator it(part.handlers.find(fd));
                if (it == part.handlers.end())
                {
                  continue;
                }
                it->second->add_reference();
                {
                  Sync::PosixGuard guard(queue_);
                  next_.push_back(it->second);
                }
                sem_.release();
              }
            }
            assert(find.descriptor() < 0);
          }
        }
        break;
      }
    }

    {
      while (!exit_)
      {
        ACE_Token::waiters_callback_(
          __gnu_cxx::__exchange_and_add(&waiters_, 1) + 1);
        __gnu_cxx::__exchange_and_add(&waiters_, 1);
        sem_.acquire(); // wait event
        __gnu_cxx::__atomic_add(&waiters_, -1);

        ACE_Event_Handler* eh = 0;
        {
          Sync::PosixGuard guard(queue_);
          if (next_.empty())
          {
            assert(exit_);
            break;
          }
          eh = next_.front();
          next_.pop_front();
        }

        assert(eh);

        {
          int fd = eh->get_handle();
          int auto_resume = eh->resume_handler() ==
            ACE_Event_Handler::ACE_REACTOR_RESUMES_HANDLER;
          int ref_count = eh->reference_counting_policy().value() ==
            ACE_Event_Handler::Reference_Counting_Policy::ENABLED;
          int status;
          while ((status = eh->handle_input(fd)) > 0)
          {
          }
          Part& part = parts_[part_(fd)];
          if (status < 0 || auto_resume)
          {
            unsigned adapted_fd = adapt_fd_for_fdset_(fd);

            Sync::PosixGuard guard(part.data); // LONG LOCK !!!
            Handlers::iterator it(part.handlers.find(adapted_fd));
            if (it != part.handlers.end() && it->second == eh)
            {
              if (status < 0)
              {
                part.handlers.erase(it);
              }
              else
              {
                FD_SET(adapted_fd, &part.wait);
                part.pipe.signal();
              }
            }
          }

          if (ref_count)
          {
            eh->remove_reference();
          }
        }
      }
      sem_.release();
    }

    if (exit_)
    {
      errno = ESHUTDOWN;
      return -1;
    }

    return 0;
  }

  int
  Reactor::alertable_handle_events(ACE_Time_Value*) noexcept
  {
    abort();
    return 0;
  }

  int
  Reactor::handle_events(ACE_Time_Value&) noexcept
  {
    abort();
    return 0;
  }

  int
  Reactor::alertable_handle_events(ACE_Time_Value&) noexcept
  {
    abort();
  }

  int
  Reactor::deactivated() noexcept
  {
    abort();
    return 0;
  }

  void
  Reactor::deactivate(int) noexcept
  {
    exit_ = true;
    for (unsigned p = 0; p < CORBACommons::PARTS; p++)
    {
      parts_[p].pipe.signal();
    }
    sem_.release();
  }

  int
  Reactor::register_handler(ACE_Event_Handler* event_handler,
    ACE_Reactor_Mask mask) noexcept
  {
    // don't register handler on mask == EXCEPT_MASK only
    // remove handler will ignore it

    if((mask & ACE_Event_Handler::READ_MASK) ||
      (mask & ACE_Event_Handler::ACCEPT_MASK))
    {
      assert(event_handler);

      int handle = event_handler->get_handle();
      Part& part = parts_[part_(handle)];

      {
        unsigned adapted_fd = adapt_fd_for_fdset_(handle);

        Sync::PosixGuard guard(part.data);
        assert(part.handlers.find(adapted_fd) == part.handlers.end());
        part.handlers[adapted_fd] = event_handler;
        FD_SET(adapted_fd, &part.wait);
      }

      part.pipe.signal();
    }

    return 0;
  }

  int
  Reactor::register_handler(ACE_HANDLE, ACE_Event_Handler*,
    ACE_Reactor_Mask) noexcept
  {
    abort();
    return 0;
  }

  int
  Reactor::register_handler(ACE_HANDLE, ACE_HANDLE, ACE_Event_Handler*,
    ACE_Reactor_Mask) noexcept
  {
    abort();
    return 0;
  }

  int
  Reactor::register_handler(const ACE_Handle_Set&, ACE_Event_Handler*,
    ACE_Reactor_Mask) noexcept
  {
    abort();
    return 0;
  }

  int
  Reactor::register_handler(int, ACE_Event_Handler*, ACE_Sig_Action*,
    ACE_Event_Handler**, ACE_Sig_Action*) noexcept
  {
    abort();
    return 0;
  }

  int
  Reactor::register_handler(const ACE_Sig_Set&, ACE_Event_Handler*,
    ACE_Sig_Action*) noexcept
  {
    abort();
    return 0;
  }

  int
  Reactor::remove_handler(ACE_Event_Handler* eh, ACE_Reactor_Mask mask)
    noexcept
  {
    remove_handler(eh->get_handle(), mask);
    return 0;
  }

  int
  Reactor::remove_handler(ACE_HANDLE handle, ACE_Reactor_Mask) noexcept
  {
    Part& part = parts_[part_(handle)];

    {
      unsigned adapted_fd = adapt_fd_for_fdset_(handle);

      Sync::PosixGuard guard(part.data);
      Handlers::iterator it(part.handlers.find(adapted_fd));
      if (it != part.handlers.end())
      {
        part.handlers.erase(it);
        FD_CLR(adapted_fd, &part.wait);
      }
    }

    part.pipe.signal();
    return 0;
  }

  int
  Reactor::remove_handler(const ACE_Handle_Set&, ACE_Reactor_Mask) noexcept
  {
    abort();
    return 0;
  }

  int
  Reactor::remove_handler(int, ACE_Sig_Action*, ACE_Sig_Action*, int)
    noexcept
  {
    abort();
    return 0;
  }

  int
  Reactor::remove_handler(const ACE_Sig_Set&) noexcept
  {
    abort();
    return 0;
  }

  int
  Reactor::suspend_handler(ACE_Event_Handler*) noexcept
  {
    abort();
    return 0;
  }

  int
  Reactor::suspend_handler(ACE_HANDLE) noexcept
  {
    abort();
    return 0;
  }

  int
  Reactor::Reactor::suspend_handler(const ACE_Handle_Set&) noexcept
  {
    abort();
    return 0;
  }

  int
  Reactor::suspend_handlers() noexcept
  {
    abort();
    return 0;
  }

  int
  Reactor::resume_handler(ACE_Event_Handler*) noexcept
  {
    abort();
    return 0;
  }

  int
  Reactor::resume_handler(ACE_HANDLE handle) noexcept
  {
    Part& part = parts_[part_(handle)];
    {
      unsigned adapted_fd = adapt_fd_for_fdset_(handle);

      Sync::PosixGuard guard(part.data);

      Handlers::iterator it(part.handlers.find(adapted_fd));
      if (it == part.handlers.end())
      {
        return -1;
      }

      FD_SET(adapted_fd, &part.wait);
      if (part.in_select)
      {
        part.pipe.signal();
      }
    }
    return 0;
  }

  int
  Reactor::resume_handler(const ACE_Handle_Set&) noexcept
  {
    abort();
    return 0;
  }

  int
  Reactor::resume_handlers() noexcept
  {
    abort();
    return 0;
  }

  int
  Reactor::resumable_handler() noexcept
  {
    return true;
  }

  bool
  Reactor::uses_event_associations() noexcept
  {
    return false;
  }

  long
  Reactor::schedule_timer(ACE_Event_Handler*, const void*,
    const ACE_Time_Value&, const ACE_Time_Value&) noexcept
  {
    // Expected that will be called only from TAO_Acceptor::handle_accept_error
    errno = EOPNOTSUPP;
    return -1;
  }

  int
  Reactor::reset_timer_interval(long, const ACE_Time_Value&) noexcept
  {
    abort();
    return 0;
  }

  int
  Reactor::cancel_timer(ACE_Event_Handler*, int) noexcept
  {
    return 0;
  }

  int
  Reactor::cancel_timer(long, const void**, int) noexcept
  {
    abort();
    return 0;
  }

  int
  Reactor::schedule_wakeup(ACE_Event_Handler*, ACE_Reactor_Mask) noexcept
  {
    abort();
    return 0;
  }

  int
  Reactor::schedule_wakeup(ACE_HANDLE, ACE_Reactor_Mask) noexcept
  {
    abort();
    return 0;
  }

  int
  Reactor::cancel_wakeup(ACE_Event_Handler*, ACE_Reactor_Mask) noexcept
  {
    abort();
    return 0;
  }

  int
  Reactor::cancel_wakeup(ACE_HANDLE, ACE_Reactor_Mask) noexcept
  {
    abort();
    return 0;
  }

  int
  Reactor::notify(ACE_Event_Handler* event_handler, ACE_Reactor_Mask mask,
    ACE_Time_Value*) noexcept
  {
    (void)mask;
    assert(mask == ACE_Event_Handler::READ_MASK);
    assert(event_handler);
    event_handler->add_reference();
    {
      Sync::PosixGuard guard(queue_);
      next_.push_back(event_handler);
    }
    sem_.release();
    return 0;
  }

  void
  Reactor::max_notify_iterations(int) noexcept
  {
    abort();
  }

  int
  Reactor::max_notify_iterations() noexcept
  {
    abort();
    return 0;
  }

  int
  Reactor::purge_pending_notifications(ACE_Event_Handler*, ACE_Reactor_Mask)
    noexcept
  {
    abort();
    return 0;
  }

  ACE_Event_Handler*
  Reactor::find_handler(ACE_HANDLE) noexcept
  {
    abort();
    return 0;
  }

  int
  Reactor::handler(ACE_HANDLE, ACE_Reactor_Mask, ACE_Event_Handler**)
    noexcept
  {
    abort();
    return 0;
  }

  int
  Reactor::handler(int, ACE_Event_Handler**) noexcept
  {
    abort();
    return 0;
  }

  bool
  Reactor::initialized() noexcept
  {
    return true;
  }

  size_t
  Reactor::size() const noexcept
  {
    abort();
    return 0;
  }

  ACE_Lock&
  Reactor::lock() noexcept
  {
    abort();
    return *(ACE_Lock*)0;
  }

  void
  Reactor::wakeup_all_threads() noexcept
  {
    abort();
  }

  int
  Reactor::owner(ACE_thread_t, ACE_thread_t*) noexcept
  {
    return 0;
  }

  int
  Reactor::owner(ACE_thread_t*) noexcept
  {
    abort();
    return 0;
  }

  bool
  Reactor::restart() noexcept
  {
    abort();
    return 0;
  }

  bool
  Reactor::restart(bool) noexcept
  {
    abort();
    return 0;
  }

  void
  Reactor::requeue_position(int) noexcept
  {
    abort();
  }

  int
  Reactor::requeue_position() noexcept
  {
    abort();
  }

  int
  Reactor::mask_ops(ACE_Event_Handler*, ACE_Reactor_Mask, int) noexcept
  {
    abort();
    return 0;
  }

  int
  Reactor::mask_ops(ACE_HANDLE, ACE_Reactor_Mask, int) noexcept
  {
    abort();
    return 0;
  }

  int
  Reactor::ready_ops(ACE_Event_Handler*, ACE_Reactor_Mask, int) noexcept
  {
    abort();
    return 0;
  }

  int
  Reactor::ready_ops(ACE_HANDLE, ACE_Reactor_Mask, int) noexcept
  {
    abort();
    return 0;
  }

  void
  Reactor::dump() const noexcept
  {
    abort();
  }
}

namespace CORBACommons
{
  ACE_Reactor_Impl*
  create_reactor_impl(ACE_Timer_Queue* tq) noexcept
  {
    try
    {
      return new Reactor(tq);
    }
    catch (...)
    {
    }
    return 0;
  }
}
