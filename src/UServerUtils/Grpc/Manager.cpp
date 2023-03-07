// BOOST
#include <boost/range/adaptor/reversed.hpp>

// USERVER
#include <userver/logging/log.hpp>

// THIS
#include "Logger.hpp"
#include "Manager.hpp"
#include "Utils.hpp"

namespace UServerUtils::Grpc
{

namespace Aspect
{

const char MANAGER[] = "MANAGER";

} // namespace Aspect

Manager::Manager(
  TaskProcessorContainerBuilderPtr&& task_processor_builder,
  ComponentsInitializeFunc&& components_initialize_func,
  const Logger_var& logger)
  : logger_(logger),
    task_processor_container_(task_processor_builder->build())
{
  if (!logger_)
  {
    Stream::Error stream;
    stream << FNS << ": logger is null";
    throw Exception(stream);
  }

  if (!task_processor_container_)
  {
    Stream::Error stream;
    stream << FNS
           << "task_processor_container is null";
    throw Exception(stream);
  }

  auto& main_task_processor =
    task_processor_container_->get_main_task_processor();
  auto& task_processor_container = *task_processor_container_;

  Logger::LoggerPtr logger_userver =
    std::make_shared<Logger::Logger>(logger_);
  auto componets_info = Utils::run_in_coro(
    main_task_processor,
    Utils::Importance::kCritical,
    {},
    [func = std::move(components_initialize_func),
     &task_processor_container,
     logger_userver = logger_userver] () {
      userver::logging::SetDefaultLogger(logger_userver);
      ComponentsBuilderPtr components_builder =
        func(task_processor_container);
      return components_builder->build();
    });

  components_ = std::move(componets_info.components);
  queue_holders_ = std::move(componets_info.queue_holders);
  statistics_storage_ = std::move(componets_info.statistics_storage);
  name_to_user_component_ = std::move(componets_info.name_to_user_component);
}

Manager::~Manager()
{
  try
  {
    Stream::Error stream;
    bool error = false;

    auto& main_task_processor =
      task_processor_container_->get_main_task_processor();

    if (state_ == AS_ACTIVE)
    {
      stream << FNS << "wasn't deactivated.";
      error = true;

      Utils::run_in_coro(
        main_task_processor,
        Utils::Importance::kCritical,
        {},
        [this] () {
          for (auto& component : components_)
          {
            try
            {
              if (component->active())
              {
                component->deactivate_object();
              }
            }
            catch (...)
            {
            }
          }
      });
    }

    Utils::run_in_coro(
      main_task_processor,
      Utils::Importance::kCritical,
      {},
      [this] () {
        name_to_user_component_.clear();
        components_.clear();
        statistics_storage_.reset();
        queue_holders_.clear();
    });

    if (state_ != AS_NOT_ACTIVE)
    {
      if (error)
      {
        stream << std::endl;
      }
      stream << FNS << "didn't wait for deactivation, still active.";
      error = true;
    }

    if (error)
    {
      logger_->error(stream.str(), Aspect::MANAGER);
    }
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
  std::lock_guard lock(state_mutex_);
  if (state_ != AS_NOT_ACTIVE)
  {
    Stream::Error stream;
    stream << FNS << "still active";
    throw ActiveObject::AlreadyActive(stream);
  }

  try
  {
    auto& main_task_processor =
      task_processor_container_->get_main_task_processor();

    Utils::run_in_coro(
      main_task_processor,
      Utils::Importance::kCritical,
      {},
      [this] () {
        try
        {
          for (auto& component : components_)
          {
            component->activate_object();
          }
          state_ = AS_ACTIVE;
        }
        catch (...)
        {
          for (auto& component : components_)
          {
            try
            {
              if (component->active())
              {
                component->deactivate_object();
              }
            }
            catch (...)
            {
            }
          }
          throw;
        }
    });
  }
  catch (const eh::Exception& exc)
  {
    Stream::Error stream;
    stream << FNS
           << "start failure: "
           << exc.what();
    throw Exception(stream);
  }
}

void Manager::deactivate_object()
{
  std::unique_lock lock(state_mutex_);
  if (state_ == AS_ACTIVE)
  {
    auto& main_task_processor =
      task_processor_container_->get_main_task_processor();

    auto exc_ptr = Utils::run_in_coro(
      main_task_processor,
      Utils::Importance::kCritical,
      {},
      [this, &main_task_processor] () {
        std::exception_ptr exc_ptr;

        for (auto& component : components_)
        {
          try
          {
            if (component->active())
            {
              component->deactivate_object();
            }
          }
          catch (...)
          {
            if (!exc_ptr)
            {
              exc_ptr = std::current_exception();
            }
          }
        }

        return exc_ptr;
    });

    state_ = AS_DEACTIVATING;
    lock.unlock();

    condition_variable_.notify_all();

    if (exc_ptr)
    {
      try
      {
        std::rethrow_exception(exc_ptr);
      }
      catch (const eh::Exception& exc)
      {
        Stream::Error stream;
        stream << FNS
               << "start failure: "
               << exc.what();
        throw Exception(stream);
      }
    }
  }
}

void Manager::wait_object()
{
  std::unique_lock lock(state_mutex_);
  condition_variable_.wait(lock, [this]() {
    return state_ != AS_ACTIVE;
  });

  for (auto& component : components_)
  {
    component->wait_object();
  }

  if (state_ == AS_DEACTIVATING)
  {
    state_ = AS_NOT_ACTIVE;
  }
}

bool Manager::active()
{
  std::lock_guard lock(state_mutex_);
  return state_ == AS_ACTIVE;
}

Manager::TaskProcessor&
Manager::get_main_task_processor()
{
  return task_processor_container_->get_main_task_processor();
}

Manager::TaskProcessor&
Manager::get_task_processor(
  const std::string& name)
{
  return task_processor_container_->get_task_processor(name);
}

} // namespace UServerUtils::Grpc