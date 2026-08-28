#pragma once

#include <HTTP/HttpTestCommons/CommonClasses.hpp>
#include <Generics/Time.hpp>
#include <vector>
#include <string>
#include <climits>

//
// class PoliciesTestInterface
//

class PoliciesTestInterface: public TestInterface
{
public:

  PoliciesTestInterface(Sync::Semaphore& finish_sem) /*throw(eh::Exception)*/;

  virtual void print_stats(std::ostream& out) /*throw(eh::Exception)*/ = 0;

  virtual void print_errors(std::ostream& out) /*throw(eh::Exception)*/ = 0;

  virtual void execute() noexcept;

protected:

  virtual ~PoliciesTestInterface() noexcept;

  virtual void exec_init_() noexcept = 0;

  virtual void exec_main_() noexcept = 0;

  virtual void exec_finish_() noexcept = 0;

  Sync::Semaphore& finish_sem_;
};

using PoliciesTestInterface_var = ReferenceCounting::QualPtr<PoliciesTestInterface>;

//
// Check policies classes
//

class CheckSimpleStatistics;
class CheckSimpleDecider;

class CheckSimpleEmptyCommons: public virtual HTTP::PoolPolicySimpleStatistics
{
public:

  using StateInfo = HTTP::PoolPolicySimpleStatistics::StateInfo;

  enum ObjectType{
    OT_CONNECTION,
    OT_THREAD
  };

  struct StateHistory
  {
    ObjectType object_type;
    int result;
    StateInfo::States state;
    Generics::Time time;
    StateHistory* next;

    StateHistory(ObjectType new_type, int new_result,
      StateInfo::States new_state, const Generics::Time& new_time) noexcept;
    StateHistory(const StateHistory& src) /*throw(eh::Exception)*/;

    ~StateHistory() noexcept;

    bool operator== (const StateHistory& src) const noexcept;
  };

  using Histories = std::map<Identifier, StateHistory>;
  using CompletedHistories = std::list<std::pair<Identifier, StateHistory> >;


  CheckSimpleEmptyCommons(unsigned int closure_delay_value) noexcept;

  static void print_state_history(const char* prefix, const void* addr,
    const StateHistory& obj, std::ostream& out) /*throw(eh::Exception)*/;

protected:

  Histories histories_;
  CompletedHistories completed_histories_;
  const StateHistory* cur_history_;
  int closure_delay_value_;
  Sync::PosixMutex mutex_;
  static Sync::PosixMutex dump_mutex_;


  virtual ~CheckSimpleEmptyCommons() noexcept;

  virtual void dynamic_states_checker_(const char* prefix, const void* addr,
    const StateHistory* prev_n_now, std::ostream& error) /*throw (eh::Exception)*/;

  //Is not protected by mutex!
  void set_history_event_(Identifier id, ObjectType type,
    StateInfo::States state, int result) /*throw (eh::Exception)*/;

  //Is not protected by mutex!
  void remove_history_(Identifier id) /*throw (eh::Exception)*/;
};

class CheckSimpleEmptyThread: public HTTP::PoolPolicySimpleEmptyThread,
                              public CheckSimpleEmptyCommons
{
public:

  CheckSimpleEmptyThread(unsigned short closure_delay = 0) noexcept;

  //Is not protected by mutex!
  virtual int when_close_thread(Identifier thread) noexcept;

  //Is not protected by mutex!
  const CompletedHistories& get_thr_history() noexcept;

protected:

  virtual ~CheckSimpleEmptyThread() noexcept;

  //Is not protected by mutex!
  StateInfo::States get_thread_state(Identifier thread) /*throw(eh::Exception)*/;

  //Is not protected by mutex!
  virtual void check_thread_connection_added(Identifier thread, Identifier connection) noexcept;

  //Is not protected by mutex!
  virtual void check_choose_thread(Identifier thread) noexcept;

  //Is not protected by mutex!
  virtual void check_thread_added(Identifier thread) noexcept;

  //Is not protected by mutex!
  virtual void check_thread_removed(Identifier thread) noexcept;


  friend class CheckSimpleStatistics;
  friend class CheckSimpleDecider;
};

class CheckSimpleEmptyConnection: public HTTP::PoolPolicySimpleEmptyConnection,
                                  public CheckSimpleEmptyCommons
{
public:

  CheckSimpleEmptyConnection(unsigned short closure_delay = 0) noexcept;

  //Is not protected by mutex!
  virtual int when_close_connection(Identifier connection) noexcept;

  //Is not protected by mutex!
  const CompletedHistories& get_conn_history() noexcept;

protected:

  virtual ~CheckSimpleEmptyConnection() noexcept;

  StateInfo::States get_connection_state(Identifier connection) /*throw(eh::Exception)*/;

  //Is not protected by mutex!
  virtual void check_connection_request_added(Identifier connection, Identifier request) noexcept;

  //Is not protected by mutex!
  virtual void
  check_choose_connection(Identifier connection, Identifier server, Identifier request) noexcept;

  //Is not protected by mutex!
  virtual void check_server_connection_added(Identifier server, Identifier connection) noexcept;

  //Is not protected by mutex!
  virtual void check_server_connection_removed(Identifier server, Identifier connection) noexcept;


  friend class CheckSimpleStatistics;
  friend class CheckSimpleDecider;
};

class CheckSimpleDecider: public HTTP::PoolPolicySimpleDecider
{
public:

  CheckSimpleDecider(int connections_per_server, int connections_per_threads,
    CheckSimpleEmptyConnection& conn_policy, CheckSimpleEmptyThread& thr_policy)
    /*throw (eh::Exception)*/;

  virtual Identifier choose_thread() noexcept;

  virtual Identifier choose_connection(Identifier server, Identifier request) noexcept;

  virtual void
  connection_request_added(Identifier server, Identifier connection, Identifier request) noexcept;

  virtual void thread_connection_added(Identifier thread, Identifier connection) noexcept;

  virtual void server_connection_added(Identifier server, Identifier connection) noexcept;

  virtual void server_connection_removed(Identifier server, Identifier connection) noexcept;

  virtual void thread_added(Identifier thread) noexcept;

  virtual void thread_removed(Identifier thread) noexcept;

private:

  CheckSimpleEmptyConnection& conn_policy_;
  CheckSimpleEmptyThread& thr_policy_;
};

//
// class ConnThrScenarios
//

class ConnThrScenarios
{
public:

  using Scenario = CheckSimpleEmptyCommons::StateHistory;
  using ScenarioArrayElem = std::pair<int, CheckSimpleEmptyCommons::StateInfo::States>;
  using Scenarios = std::vector<Scenario>;
  using ScenariosCompleted = std::vector<char>;

  ConnThrScenarios() /*throw(eh::Exception)*/;

  void add_scenario(const Scenario& new_scen) /*throw(eh::Exception)*/;

  void add_scenario(CheckSimpleEmptyCommons::ObjectType type,
    const ScenarioArrayElem* new_scen, size_t length) /*throw(eh::Exception)*/;

  bool check_conn_scenario(const Scenario& new_scen) /*throw(eh::Exception)*/;

  bool check_thr_scenario(const Scenario& new_scen) /*throw(eh::Exception)*/;

  const ScenariosCompleted& conn_scens_completed() noexcept;

  const ScenariosCompleted& thr_scens_completed() noexcept;

  bool all_completed(std::ostringstream& log) /*throw(eh::Exception)*/;

  void print_scenario(std::ostream& log, const Scenario* scen)
    /*throw(eh::Exception)*/;

private:

  Scenarios conn_scenarios_;
  Scenarios thr_scenarios_;
  ScenariosCompleted thr_scens_completed_;
  ScenariosCompleted conn_scens_completed_;
};
