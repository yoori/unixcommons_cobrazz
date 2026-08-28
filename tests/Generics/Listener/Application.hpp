// Application.hpp
#pragma once

  /**
   * Special callback with checks and correctness control abilities.
   */
class DescriptorListenerCallbackTester :
  public virtual Generics::ActiveDescriptorListenerCallback,
  public virtual Generics::DescriptorListenerCallback,
  public virtual Generics::ExecuteAndListenCallback,
  public ReferenceCounting::AtomicImpl
{
public:
  DescriptorListenerCallbackTester() noexcept;

  virtual void
  on_data_ready(int fd, std::size_t fd_index, const char* str, std::size_t size) noexcept;

  virtual void on_closed(int fd, std::size_t fd_index, int error) noexcept;

  virtual void on_all_closed() noexcept;

  std::size_t get_and_reset_closed() noexcept;

  std::string received_data() const noexcept;

  void reset() noexcept;

  void set_full_lines_test(bool new_value) noexcept;

  virtual void
  report_error(Severity severity, const String::SubString& description,
    const char* error_code = 0) noexcept;
protected:
  virtual ~DescriptorListenerCallbackTester() noexcept;
private:
  volatile _Atomic_word close_counter_;
  std::string ready_data_;
  Sync::PosixMutex mutex_;
  int checking_descriptor_;
  bool full_lines_test_;
};

  using DescriptorListenerCallbackTester_var =
    ReferenceCounting::QualPtr<DescriptorListenerCallbackTester>;

class TestTasker
{
public:
  using Descriptors = Generics::ArrayAutoPtr<int>;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  TestTasker() /*throw (eh::Exception)*/;

  virtual ~TestTasker() noexcept;

  /**
   * Check all data delivery.
   */
  void do_overflow_test(bool buffering_mode) /*throw (eh::Exception)*/;

  /**
   * Close half of descriptors amount. Check correct of quantity.
   */
  void do_closed_descriptors_test(bool buffering_mode) /*throw (eh::Exception)*/;

  /**
   * Check all data delivery + full lines mode only.
   */
  void do_auto_test(bool buffering_mode) /*throw (eh::Exception)*/;

  /**
   * Do execute_and_listen function test
   */
  void do_execute_and_listen_test(const char* program_name)
    /*throw (eh::Exception)*/;

private:

  void
  spawn_descriptors_(Descriptors& read_descriptors,
    Descriptors& write_descriptors,
    std::size_t count)
    /*throw (eh::Exception)*/;

  static const std::size_t PIPES_COUNT_ = 10;

  Descriptors read_descriptors;
  Descriptors write_descriptors;
  DescriptorListenerCallbackTester_var callback_;
};

using Descriptors = TestTasker::Descriptors;

/**
 * Child part for execute_and_listen function test
 */
void do_execute_and_listen_test_child_code(char* argv[])
  /*throw (eh::Exception)*/;

class Writer
{
public:
  Writer(Descriptors& dscs, const char* msg) noexcept;

  void operator()() /*throw (eh::Exception)*/;

  /**
   * Should reset multiplexer before new test cycle.
   */
  void reset() noexcept;

private:

  Descriptors& write_pipes_;
  const std::string MSG_;

  // use it for choosing pipe by some thread.
  volatile _Atomic_word multiplexor_;
};

class MTAdapter
{
public:
  MTAdapter(const char* progname) noexcept;
  void operator ()() /*throw (eh::Exception)*/;

private:
  const char* progname;
};

class MPAdapter
{
public:
  MPAdapter(const char* progname, int threads, time_t interval, int limit = -1) noexcept;
  void operator ()() /*throw (eh::Exception)*/;

private:
  const char* progname;
  int threads;
  time_t interval;
  int limit;
};
