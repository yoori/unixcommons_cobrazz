#pragma once

#include <eh/Exception.hpp>
#include <Sync/PosixLock.hpp>

#include <TestInt_s.hpp>

#include <CORBACommons/ServantImpl.hpp>
#include <CORBACommons/StatsImpl.hpp>

namespace CORBATest
{
  class TestIntImpl :
    virtual public
      CORBACommons::ReferenceCounting::ServantImpl<POA_CORBATest::TestInt>,
    private CORBACommons::ProcessStatsImpl
  {
  public:
    struct Callback
    {
      virtual ~Callback() noexcept
      {
      }
      virtual void error(const char*) noexcept = 0;
    };

    TestIntImpl(int seq3 = 3000, int seq2 = 15, int size = 1000) noexcept;

    virtual ~TestIntImpl() noexcept;

    virtual void test(const OctetSeq& in_seq) noexcept;

    virtual void oneway_test(const OctetSeq& in_seq) noexcept;

    virtual Seq3* memory_test() /*throw (eh::Exception)*/;

    virtual void print_memory(CORBA::Boolean full) noexcept;

    volatile _Atomic_word received_requests;

  private:
    unsigned timeout_;
    int seq3_, seq2_, size_;
  };

  using TestIntImpl_var = ReferenceCounting::QualPtr<TestIntImpl>;
}
