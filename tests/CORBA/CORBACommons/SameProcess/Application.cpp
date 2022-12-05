#include "TestInt2_s.hpp"

#include "../Init/Init.hpp"

const char TEST_INT[] = "TestInt";

namespace CORBATest
{
  class TestIntImpl :
    virtual public
      CORBACommons::ReferenceCounting::ServantImpl<POA_CORBATest::TestInt>
  {
  public:
    virtual
    void
    test() throw ()
    {
    }

  protected:
    virtual
    ~TestIntImpl() throw ()
    {
    }
  };
  typedef ReferenceCounting::QualPtr<TestIntImpl> TestIntImpl_var;
}

void
Client::work() /*throw (eh::Exception)*/
{
}

class Client1 : public Client
{
public:
  void
  run() /*throw (eh::Exception)*/
  {
    sleep(1);
  }
};

class Server1 : public Server
{
public:
  virtual
  void
  init_endpoint(CORBACommons::EndpointConfig& endpoint_config)
    /*throw (eh::Exception)*/
  {
    endpoint_config.objects[TEST_INT].insert(TEST_INT);
  }

  virtual
  void
  work() /*throw (eh::Exception)*/
  {
    Server::work();
    test_int_impl_ = new CORBATest::TestIntImpl();
    corba_server_adapter->add_binding(TEST_INT, test_int_impl_);
    shutdowner_ = corba_server_adapter->shutdowner();
    pthread_create(&th, 0, thread, this);
  }

  void
  stop() /*throw (eh::Exception)*/
  {
    shutdowner_->shutdown(true);
    shutdowner_.reset();
    pthread_join(th, 0);
  }

private:
  static
  void*
  thread(void* arg)
  {
    static_cast<Server1*>(arg)->corba_server_adapter->run();
    return 0;
  }

  CORBATest::TestIntImpl_var test_int_impl_;
  CORBACommons::OrbShutdowner_var shutdowner_;
  pthread_t th;
};

template <typename Client, typename Server>
class Usage1 : public Usage<Client, Server>
{
public:
  virtual
  void
  action(Client& client, Server& server) /*throw (eh::Exception)*/
  {
    client.run();
    server.stop();
  }
};

int
main(int argc, char* argv[])
{
  Usage1<Client1, Server1> usage;
  return usage.use(argc, argv);
}
