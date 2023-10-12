// BOOST
#include <boost/range/adaptor/reversed.hpp>

// USERVER
#include <userver/logging/log.hpp>

// THIS
#include <UServerUtils/Grpc/Logger.hpp>
#include <UServerUtils/Grpc/Manager.hpp>
#include <UServerUtils/Grpc/Utils.hpp>

namespace UServerUtils::Grpc
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

  auto componets_info = Utils::run_in_coro(
    main_task_processor,
    Utils::Importance::kCritical,
    {},
    [func = std::move(components_initialize_func),
     &task_processor_container,
     this] () mutable {
      logger_scope_ =
        std::make_unique<UServerUtils::Grpc::Logger::LoggerScope>(logger_.in());
      ComponentsBuilderPtr components_builder =
        func(task_processor_container);
      return components_builder->build();
    });

  components_ = std::move(componets_info.components);
  queue_holders_ = std::move(componets_info.queue_holders);
  statistics_storage_ = std::move(componets_info.statistics_storage);
  name_to_user_component_ = std::move(componets_info.name_to_user_component);
  middlewares_list_ = std::move(componets_info.middlewares_list);
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
        auto it = std::rbegin(components_);
        auto it_end = std::rend(components_);
        for (; it != it_end; ++it)
        {
          it->reset();
        }
        components_.clear();
        statistics_storage_.reset();
        queue_holders_.clear();
        middlewares_list_.clear();
        logger_scope_.reset();
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

  auto& main_task_processor =
    task_processor_container_->get_main_task_processor();
  Utils::run_in_coro(
    main_task_processor,
    Utils::Importance::kCritical,
    {},
    [this, &main_task_processor] () {
      for (auto& component : components_)
      {
        try
        {
          component->wait_object();
        }
        catch (...)
        {
        }
      }
    });

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