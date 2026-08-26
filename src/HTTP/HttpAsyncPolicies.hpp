#pragma once

#include <map>

#include <Sync/Semaphore.hpp>

#include <HTTP/HttpAsyncPool.hpp>


namespace HTTP
{
  //Forward declarations
  class EmptyPoliciesCommonMethods;


  //
  // class PoolPolicySimpleStatistics
  //

  class PoolPolicySimpleStatistics : public virtual PoolPolicyStatistics
  {
  public:
    virtual
    void
    server_added(Identifier server) noexcept;

    virtual
    void
    server_removed(Identifier server) noexcept;

    virtual
    void
    server_connection_added(Identifier server, Identifier connection)
      noexcept;

    virtual
    void
    server_connection_removed(Identifier server, Identifier connection)
      noexcept;


    virtual
    void
    thread_added(Identifier thread) noexcept;

    virtual
    void
    thread_removed(Identifier thread) noexcept;

    virtual
    void
    thread_connection_added(Identifier thread, Identifier connection)
      noexcept;

    virtual
    void
    thread_connection_removed(Identifier thread, Identifier connection)
      noexcept;


    virtual
    void
    connection_request_added(Identifier server, Identifier connection,
      Identifier request) noexcept;

    virtual
    void
    connection_request_removed(Identifier connection, Identifier request)
      noexcept;

    virtual
    void
    server_request_added(Identifier server, Identifier request)
      noexcept;

    virtual
    void
    server_request_removed(Identifier server, Identifier request)
      noexcept;

  protected:
    virtual
    void
    server_added_i(Identifier server) /*throw (eh::Exception)*/;

    virtual
    void
    server_removed_i(Identifier server) /*throw (eh::Exception)*/;

    virtual
    void
    server_connection_added_i(Identifier server, Identifier connection)
      /*throw (eh::Exception)*/;

    virtual
    void
    server_connection_removed_i(Identifier server, Identifier connection)
      /*throw (eh::Exception)*/;


    virtual
    void
    thread_removed_i(Identifier thread) /*throw (eh::Exception)*/;

    virtual
    void
    thread_connection_added_i(Identifier thread, Identifier connection)
      /*throw (eh::Exception)*/;

    virtual
    void
    thread_connection_removed_i(Identifier thread, Identifier connection)
      /*throw (eh::Exception)*/;


    virtual
    void
    connection_request_added_i(Identifier server, Identifier connection,
      Identifier request) /*throw (eh::Exception)*/;

    virtual
    void
    connection_request_removed_i(Identifier connection, Identifier request)
      /*throw (eh::Exception)*/;

  protected:
    struct StateInfo
    {
      enum States
      {
        ACTIVE_AWAITING,
        ACTIVE,
        CLOSURE_AWAITING,
        CLOSURE_ON_NEXT_TRY,
        CLOSING
      };

      mutable States state;

      StateInfo() noexcept;
    };

    struct SimpleStat : public StateInfo
    {
      unsigned items_count;

      SimpleStat() noexcept;
    };

    struct Connection : public SimpleStat
    {
      Identifier server;
      Identifier thread;

      Connection() noexcept;
      Connection(Identifier server) noexcept;
    };
    typedef std::map<Identifier, Connection> Connections;

    struct Thread : public SimpleStat
    {
      mutable bool full;

      Thread() noexcept;
    };
    typedef std::map<Identifier, Thread> Threads;

    typedef std::map<Identifier, Connections::iterator> ConnectionPtrs;
    typedef std::map<Identifier, ConnectionPtrs> Servers;

    virtual
    ~PoolPolicySimpleStatistics() noexcept;

    const Servers&
    get_servers_() const noexcept;

    const Threads&
    get_threads_() const noexcept;

    const Connections&
    get_connections_() const noexcept;

  private:
    Servers servers_;
    Threads threads_;
    Connections connections_;
  };


  //
  // class PoolPolicyAdvancedStatistics
  //

  class PoolPolicyAdvancedStatistics :
    public virtual PoolPolicySimpleStatistics
  {
  public:
    virtual
    void
    server_request_added(Identifier server, Identifier request) noexcept;

    virtual
    void
    server_request_removed(Identifier server, Identifier request) noexcept;

    virtual
    void
    server_added_i(Identifier server) /*throw (eh::Exception)*/;

    virtual
    void
    server_removed_i(Identifier server) /*throw (eh::Exception)*/;

    virtual
    void
    connection_request_added_i(Identifier server, Identifier connection,
      Identifier request) /*throw (eh::Exception)*/;

  protected:
    virtual
    ~PoolPolicyAdvancedStatistics() noexcept;

    typedef std::map<Identifier, int> Requests;
    typedef std::map<Identifier, Requests> ServerRequests;

    ServerRequests server_requests_;
  };


  //
  // class PoolPolicySimpleDecider
  //

  class PoolPolicySimpleDecider :
    public virtual PoolPolicySimpleStatistics,
    public virtual PoolPolicyDecider
  {
  public:
    PoolPolicySimpleDecider(unsigned connections_per_server,
      unsigned connections_per_threads)
      /*throw (eh::Exception)*/;

    virtual
    Identifier
    choose_thread() noexcept;

    virtual
    Identifier
    choose_connection(Identifier server, Identifier request) noexcept;

    virtual
    RequestPolicy
    request_failed(Identifier server, Identifier request) noexcept;

    virtual
    RequestPolicy
    requests_failed(Identifier server) noexcept;

  private:
    const unsigned CONNECTIONS_PER_SERVER_;
    const unsigned CONNECTIONS_PER_THREADS_;
  };


  //
  // class PoolPolicySimpleEmptyThread
  //

  class PoolPolicySimpleEmptyThread :
    public virtual PoolPolicySimpleStatistics,
    public virtual PoolPolicyEmptyThread
  {
  public:
    PoolPolicySimpleEmptyThread(time_t closure_delay = 3)
      noexcept;

    virtual
    int
    when_close_thread(Identifier thread) noexcept;

  protected:
    virtual
    ~PoolPolicySimpleEmptyThread() noexcept;

    int
    process_active_(Threads::const_iterator& cur_thr,
      const Threads&) /*throw (eh::Exception)*/;

    int
    process_closure_awaiting_(Threads::const_iterator& cur_thr,
      const Threads&) /*throw (eh::Exception)*/;

  private:
    const time_t CLOSURE_DELAY_;

    friend class EmptyPoliciesCommonMethods;
  };


  //
  // class PoolPolicySimpleEmptyConnection
  //

  class PoolPolicySimpleEmptyConnection :
    public virtual PoolPolicySimpleStatistics,
    public virtual PoolPolicyEmptyConnection
  {
  public:
    PoolPolicySimpleEmptyConnection(time_t closure_delay = 3)
      noexcept;

    virtual
    int
    when_close_connection(Identifier connection) noexcept;

  protected:
    virtual
    ~PoolPolicySimpleEmptyConnection() noexcept;

    int
    process_active_(Connections::const_iterator& conn_it,
      const ConnectionPtrs& aux_map) /*throw (eh::Exception)*/;

    int
    process_closure_awaiting_(Connections::const_iterator& conn_it,
      const ConnectionPtrs& aux_map) /*throw (eh::Exception)*/;

  private:
    const time_t CLOSURE_DELAY_;

    friend class EmptyPoliciesCommonMethods;
  };


  //
  // class PoolPolicySimpleRequests
  //

  class PoolPolicySimpleRequests : public virtual PoolPolicyRequests
  {
  public:
    virtual
    void
    request_constructing() /*throw (eh::Exception)*/;

    virtual
    void
    request_destroying() noexcept;

  protected:
    virtual
    ~PoolPolicySimpleRequests() noexcept;
  };


  //
  // class PoolPolicyWaitRequests
  //

  class PoolPolicyWaitRequests : public virtual PoolPolicyRequests
  {
  public:
    PoolPolicyWaitRequests(unsigned requests) /*throw (eh::Exception)*/;

    virtual
    void
    request_constructing() /*throw (eh::Exception)*/;

    virtual
    void
    request_destroying() noexcept;

  protected:
    virtual
    ~PoolPolicyWaitRequests() noexcept;

  private:
    Sync::Semaphore semaphore_;
  };


  //
  // class PoolPolicyThrowRequests
  //

  class PoolPolicyThrowRequests : public virtual PoolPolicyRequests
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    PoolPolicyThrowRequests(unsigned requests) /*throw (eh::Exception)*/;

    virtual
    void
    request_constructing() /*throw (eh::Exception)*/;

    virtual
    void
    request_destroying() noexcept;

  protected:
    virtual
    ~PoolPolicyThrowRequests() noexcept;

  private:
    volatile _Atomic_word requests_;
  };


  //
  // class PoolPolicySimpleTimeout
  //

  class PoolPolicySimpleTimeout : public virtual PoolPolicyTimeout
  {
  public:
    PoolPolicySimpleTimeout(const time_t timeout = 0) noexcept;

    virtual
    int
    expiration_timeout(Identifier connection) noexcept;

  protected:
    virtual
    ~PoolPolicySimpleTimeout() noexcept;

  private:
    const time_t TIMEOUT_;
  };
}
