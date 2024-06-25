// BOOST
#include <boost/range/adaptor/reversed.hpp>

// USERVER
#include <userver/logging/log.hpp>

// THIS
#include <UServerUtils/Logger.hpp>
#include <UServerUtils/Manager.hpp>
#include <UServerUtils/Utils.hpp>

namespace UServerUtils
{

namespace Aspect
{

const char MANAGER[] = "MANAGER";

} // namespace Aspect

Manager::Manager(
  TaskProcessorContainerBuilderPtr&& task_processor_builder,
  ComponentsInitializeFunc&& components_initialize_func,
  Logger* logger)
  : logger_(ReferenceCounting::add_ref(logger)),
    task_processor_container_(task_processor_builder->build())
{
  using LoggerScope = UServerUtils::Logger::LoggerScope;

  if (!logger)
  {
    Stream::Error stream;
    stream << FNS
           << "logger is null";
    throw Exception(stream);
  }

  if (!task_processor_container_)
  {
    Stream::Error stream;
    stream << FNS
           << "task_processor_container is null";
    throw Exception(stream);
  }

  auto& main_task_processor = task_processor_container_->get_main_task_processor();
  auto& task_processor_container = *task_processor_container_;

  coro_data_container_ = Utils::run_in_coro(
    main_task_processor,
    Utils::Importance::kCritical,
    {},
    [func = std::move(components_initialize_func),
     &task_processor_container,
     logger = logger_,
     this] () mutable {
      ComponentsBuilderPtr components_builder = func(task_processor_container);
      auto coro_data_container = std::make_unique<CoroDataContainer>();
      coro_data_container->logger_scope =
        std::make_unique<LoggerScope>(logger.in());
      auto componets_info = components_builder->build();
      coro_data_container->statistics_holders =
        std::move(componets_info.statistics_holders);
      coro_data_container->queue_holders =
        std::move(componets_info.queue_holders);
      coro_data_container->statistics_storage =
        std::move(componets_info.statistics_storage);
      coro_data_container->name_to_user_component =
        std::move(componets_info.name_to_user_component);
      coro_data_container->middlewares_list =
        std::move(componets_info.middlewares_list);

      for (auto& component : componets_info.components)
      {
        add_child_object(component.in());
      }

      return coro_data_container;
    });
}

Manager::~Manager()
{
  try
  {
    auto& main_task_processor = task_processor_container_->get_main_task_processor();
    Utils::run_in_coro(
      main_task_processor,
      Utils::Importance::kCritical,
      {},
      [this] () {
        try
        {
          Generics::CompositeActiveObject::clear_children();
        }
        catch (...)
        {
        }
        coro_data_container_.reset();
    });
  }
  catch (const eh::Exception& exc)
  {
    try
    {
      std::cerr << FNS << "eh::Exception: " << exc.what() << std::endl;
    }
    catch (...)
    {
    }
  }

  task_processor_container_.reset();
}

void Manager::activate_object()
{
  try
  {
    auto& main_task_processor = task_processor_container_->get_main_task_processor();
    Utils::run_in_coro(
      main_task_processor,
      Utils::Importance::kCritical,
      {},
      [this] () {
        Generics::CompositeActiveObject::activate_object();
    });
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error stream;
    stream << FNS
           << "activate failure: "
           << exc.what();
    throw Exception(stream);
  }
}

void Manager::deactivate_object()
{
  try
  {
    auto& main_task_processor = task_processor_container_->get_main_task_processor();
    Utils::run_in_coro(
      main_task_processor,
      Utils::Importance::kCritical,
        {},
        [this] () {
          Generics::CompositeActiveObject::deactivate_object();
      });
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error stream;
    stream << FNS
           << "deactivate failure: "
           << exc.what();
    throw Exception(stream);
  }
}

void Manager::wait_object()
{
  try
  {
    auto& main_task_processor = task_processor_container_->get_main_task_processor();
    Utils::run_in_coro(
      main_task_processor,
      Utils::Importance::kCritical,
      {},
      [this] () {
        Generics::CompositeActiveObject::wait_object();
      });
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error stream;
    stream << FNS
           << "wait_object failure: "
           << exc.what();
    throw Exception(stream);
  }
}

Manager::TaskProcessor& Manager::get_main_task_processor()
{
  return task_processor_container_->get_main_task_processor();
}

Manager::TaskProcessor& Manager::get_task_processor(const std::string& name)
{
  return task_processor_container_->get_task_processor(name);
}

} // namespace UServerUtil