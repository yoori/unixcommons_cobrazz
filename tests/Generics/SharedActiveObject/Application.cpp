#include <Generics/SharedActiveObject.hpp>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace
{
  using namespace std::chrono_literals;

  void require(bool condition, const char* message)
  {
    if (!condition)
    {
      std::cerr << message << std::endl;
      std::abort();
    }
  }

  void await(std::future<void>& future)
  {
    require(future.wait_for(5s) == std::future_status::ready, "completion timed out");
    future.get();
  }

  class Resource final: public Generics::SharedActiveObject
  {
  public:
    std::atomic<unsigned int> starts{0};
    std::atomic<unsigned int> stops{0};
    std::function<void()> on_stop;
    bool fail_start = false;

  private:
    void activate_shared_object_() override;
    void wait_object_() override;
  };

  void Resource::activate_shared_object_()
  {
    require(active(), "activation hook must see the active state");
    if (fail_start)
    {
      throw std::runtime_error("start failed");
    }

    ++starts;
  }

  void Resource::wait_object_()
  {
    ++stops;
    if (on_stop)
    {
      on_stop();
    }
  }

  class Consumer final: public Generics::SimpleActiveObject
  {
  public:
    explicit Consumer(std::shared_ptr<Resource> resource);

  private:
    void wait_object_() override;

    Generics::SharedActiveObject::ActivateGuard guard_;
  };

  Consumer::Consumer(std::shared_ptr<Resource> resource)
    : guard_(resource->activate_guard())
  {}

  void Consumer::wait_object_()
  {
    guard_.reset();
  }

  void check_closed(const std::shared_ptr<Resource>& resource)
  {
    try
    {
      resource->activate_guard();
      require(false, "guard issued after deactivation");
    }
    catch (const Generics::ActiveObject::NotSupported&)
    {}
  }

  void test_explicit_activation()
  {
    auto resource = std::make_shared<Resource>();
    resource->wait_object();
    require(resource->stops == 0, "wait started an unused resource");
    resource->activate_object();
    require(resource->active() && resource->starts == 1, "resource did not start");
    auto waiting = std::async(std::launch::async, [resource] { resource->wait_object(); });
    require(waiting.wait_for(20ms) == std::future_status::timeout, "wait ignored active state");
    resource->deactivate_object();
    await(waiting);
    resource->deactivate_object();
    resource->wait_object();
    require(resource->stops == 1 && !resource->active(), "shutdown must run once");
    check_closed(resource);
    try
    {
      resource->activate_object();
      require(false, "stopped resource was restarted");
    }
    catch (const Generics::ActiveObject::AlreadyActive&)
    {}
  }

  void test_reserved_activity_shutdown()
  {
    auto resource = std::make_shared<Resource>();
    Consumer consumer(resource);
    resource->deactivate_object();
    check_closed(resource);
    resource->wait_object();
    require(resource->stops == 0, "reserved activity was ignored");
    consumer.wait_object();
    resource->wait_object();
    require(resource->starts == 0 && resource->stops == 1,
      "shutdown before explicit activation was not completed once");
  }

  void test_consumer_order(bool resource_first)
  {
    auto resource = std::make_shared<Resource>();
    Consumer first(resource);
    Consumer second(resource);
    resource->activate_object();
    resource->deactivate_object();
    check_closed(resource);
    require(resource->stops == 0, "deactivate performed final shutdown");
    if (resource_first)
    {
      resource->wait_object();
      resource->wait_object();
    }

    first.wait_object();
    first.wait_object();
    require(resource->stops == 0, "shutdown ignored a remaining consumer");
    second.wait_object();
    if (!resource_first)
    {
      require(resource->stops == 0, "guard consumed the explicit activation");
    }

    resource->wait_object();
    require(resource->stops == 1, "last owner did not finish shutdown");
  }

  void test_wait_for_running_hook()
  {
    auto resource = std::make_shared<Resource>();
    Consumer consumer(resource);
    std::promise<void> entered;
    auto started = entered.get_future();
    std::promise<void> release;
    auto resume = release.get_future().share();
    resource->on_stop = [&entered, resume]
      {
        entered.set_value();
        resume.wait();
      };
    resource->activate_object();
    resource->deactivate_object();
    resource->wait_object();
    auto stopping = std::async(std::launch::async, [&consumer] { consumer.wait_object(); });
    await(started);
    auto waiting = std::async(std::launch::async, [resource] { resource->wait_object(); });
    require(waiting.wait_for(20ms) == std::future_status::timeout,
      "wait returned before final shutdown completed");
    release.set_value();
    await(stopping);
    await(waiting);
    require(resource->stops == 1, "concurrent wait repeated final shutdown");
  }

  void test_concurrent_consumers()
  {
    auto resource = std::make_shared<Resource>();
    std::vector<std::unique_ptr<Consumer>> consumers;
    for (unsigned int i = 0; i < 16; ++i)
    {
      consumers.emplace_back(std::make_unique<Consumer>(resource));
    }

    resource->activate_object();
    resource->deactivate_object();
    std::vector<std::future<void>> waits;
    for (const auto& consumer : consumers)
    {
      waits.emplace_back(std::async(std::launch::async,
        [client = consumer.get()] { client->wait_object(); }));
    }

    for (unsigned int i = 0; i < 8; ++i)
    {
      waits.emplace_back(std::async(std::launch::async, [resource] { resource->wait_object(); }));
    }

    for (auto& wait : waits)
    {
      await(wait);
    }

    require(resource->stops == 1, "concurrent releases did not finalize exactly once");
  }

  void test_lifetime_and_move()
  {
    auto resource = std::make_shared<Resource>();
    auto first = resource->activate_guard();
    auto second = std::move(first);
    require(!first && second, "guard move did not transfer ownership");
    first = std::move(second);
    require(first && !second, "guard assignment did not transfer ownership");
    Consumer consumer(resource);
    first.reset(); // A second guard still owns the activity; no blocking shutdown here.
    resource->activate_object();
    resource->deactivate_object();
    resource->wait_object();
    std::weak_ptr<Resource> weak = resource;
    resource.reset();
    require(!weak.expired(), "guard did not retain the resource");
    consumer.wait_object();
    require(weak.expired(), "guard leaked the resource");
  }

  void test_hook_failure()
  {
    auto resource = std::make_shared<Resource>();
    resource->fail_start = true;
    try
    {
      resource->activate_object();
      require(false, "activation failure lost");
    }
    catch (const std::runtime_error&)
    {}

    resource->fail_start = false;
    resource->activate_object();
    Consumer consumer(resource);
    resource->on_stop = [] { throw std::runtime_error("stop failed"); };
    resource->deactivate_object();
    resource->wait_object();
    consumer.wait_object();
    for (unsigned int i = 0; i < 2; ++i)
    {
      try
      {
        resource->wait_object();
        require(false, "final shutdown failure lost");
      }
      catch (const std::runtime_error&)
      {}
    }

    require(resource->stops == 1, "failed shutdown was repeated");
  }
}

int main()
{
  test_explicit_activation();
  test_reserved_activity_shutdown();
  test_consumer_order(false);
  test_consumer_order(true);
  test_wait_for_running_hook();
  test_concurrent_consumers();
  test_lifetime_and_move();
  test_hook_failure();
  std::cout << "TestSharedActiveObject passed" << std::endl;
}
