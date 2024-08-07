#ifndef USERVER_MANAGER_HPP
#define USERVER_MANAGER_HPP

// STD
#include <functional>
#include <memory>
#include <mutex>
#include <condition_variable>

// THIS
#include <eh/Exception.hpp>
#include <Logger/Logger.hpp>
#include <UServerUtils/Statistics/StatisticsProvider.hpp>
#include <UServerUtils/ComponentsBuilder.hpp>
#include <UServerUtils/Logger.hpp>
#include <UServerUtils/Function.hpp>
#include <UServerUtils/TaskProcessorContainerBuilder.hpp>

namespace UServerUtils
{

class Manager final :
  public Generics::CompositeActiveObject,
  public ReferenceCounting::AtomicImpl
{
public:
  using ComponentsInitializeFunc =
    Utils::Function<ComponentsBuilderPtr(
      TaskProcessorContainer& task_processor_container)>;

  using TaskProcessor = userver::engine::TaskProcessor;
  using Components = std::deque<Component_var>;
  using StatisticsStorage = userver::utils::statistics::Storage;
  using StatisticsStoragePtr = std::unique_ptr<StatisticsStorage>;
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using Exception = ActiveObject::Exception;
  using AlreadyActive = ActiveObject::AlreadyActive;

private:
  using LoggerScope = UServerUtils::Logger::LoggerScope;
  using LoggerScopePtr = UServerUtils::Logger::LoggerScopePtr;
  using QueueHolder = userver::ugrpc::client::QueueHolder;
  using QueueHolderPtr = std::unique_ptr<QueueHolder>;
  using QueueHolders = std::deque<QueueHolderPtr>;
  using NameToUserComponent = std::unordered_map<std::string, Component_var>;
  using Middlewares = userver::ugrpc::server::Middlewares;
  using MiddlewaresPtr = std::unique_ptr<Middlewares>;
  using MiddlewaresList = std::list<MiddlewaresPtr>;
  using StatisticsHolder = userver::utils::statistics::Entry;
  using StatisticsHolderPtr = std::unique_ptr<StatisticsHolder>;
  using StatisticsHolders = std::list<StatisticsHolderPtr>;

  struct CoroDataContainer final : private Generics::Uncopyable
  {
    LoggerScopePtr logger_scope;
    MiddlewaresList middlewares_list;
    QueueHolders queue_holders;
    StatisticsStoragePtr statistics_storage;
    NameToUserComponent name_to_user_component;
    StatisticsHolders statistics_holders;
  };
  using CoroDataContainerPtr = std::unique_ptr<CoroDataContainer>;

public:
  explicit Manager(
    TaskProcessorContainerBuilderPtr&& task_processor_builder,
    ComponentsInitializeFunc&& components_initialize_func,
    Logger* logger);

  void activate_object() override;

  void deactivate_object() override;

  void wait_object() override;

  TaskProcessor& get_main_task_processor();

  TaskProcessor& get_task_processor(const std::string& name);

  template<typename T>
  T& get_user_component(const std::string& name)
  {
    auto& name_to_user_component =
      coro_data_container_->name_to_user_component;
    auto it = name_to_user_component.find(name);
    if (it == name_to_user_component.end())
    {
      Stream::Error stream;
      stream << FNS
             << " : user component with name="
             << name
             << " not exist";
      throw Exception(stream);
    }

    Component* component = it->second.in();
    T* p = dynamic_cast<T*>(component);
    if (!p)
    {
      Stream::Error stream;
      stream << FNS
             << " : can't cast to type T";
      throw Exception(stream);
    }

    return *p;
  }

protected:
  ~Manager() override;

private:
  const Logger_var logger_;

  TaskProcessorContainerPtr task_processor_container_;

  CoroDataContainerPtr coro_data_container_;
};

using Manager_var = ReferenceCounting::SmartPtr<Manager>;

} // namespace UServerUtils

#endif //USERVER_MANAGER_HPP