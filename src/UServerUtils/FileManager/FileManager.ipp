// STD
#include <future>
#include <iostream>

// USERVER
#include <userver/engine/future.hpp>
#include <userver/engine/task/task.hpp>

// THIS
#include <Generics/Function.hpp>
#include <UServerUtils/FileManager/FileManager.hpp>
#include <UServerUtils/FileManager/Utils.hpp>

namespace UServerUtils::FileManager
{

namespace Aspect
{

inline constexpr char FILE_MANAGER[] = "FILE_MANAGER";

} // namespace Aspect

inline FileManager::FileManager(
  const Config& config,
  Logger* logger)
  : logger_(ReferenceCounting::add_ref(logger)),
    event_queue_(std::make_shared<EventQueue>(
      config.event_queue_max_size)),
    semaphore_(std::make_shared<Semaphore>(Semaphore::Type::Blocking, 0))
{
  auto uring = std::make_unique<IoUring>(config);
  uring_fd_ = uring->get()->ring_fd;
  initialize(std::move(uring));
}

inline FileManager::FileManager(
  const Config& config,
  const std::uint32_t uring_fd,
  Logger* logger)
  : logger_(ReferenceCounting::add_ref(logger)),
    event_queue_(std::make_shared<EventQueue>(
      config.event_queue_max_size)),
    semaphore_(std::make_shared<Semaphore>(Semaphore::Type::Blocking, 0))
{
  auto uring = std::make_unique<IoUring>(config, uring_fd);
  uring_fd_ = uring_fd;
  initialize(std::move(uring));
}

inline void FileManager::initialize(IoUringPtr&& uring)
{
  max_size_cq_queue_ = uring->cq_size();
  if (!create_semaphore_event(semaphore_->fd(), uring->get()))
  {
    std::ostringstream stream;
    stream << FNS
           << "create_eventfd_read_event is failed";
    throw Exception(stream.str());
  }

  thread_ = std::make_unique<Thread>(
    &FileManager::run,
    this,
    semaphore_,
    std::move(uring));
}

inline std::uint32_t FileManager::uring_fd() const noexcept
{
  return uring_fd_;
}

inline FileManager::~FileManager()
{
  try
  {
    Event close_event;
    close_event.type = EventType::Close;
    while (!event_queue_->emplace(std::move(close_event)))
    {
      std::this_thread::sleep_for(
        std::chrono::milliseconds(100));
    }

    const std::size_t max_attempts = 5;
    std::size_t i = 0;
    while (!semaphore_->add() && i < max_attempts)
    {
      std::this_thread::sleep_for(
        std::chrono::milliseconds(100));
      i += 1;
    }
    if (i == max_attempts)
    {
      std::ostringstream stream;
      stream << FNS
             << "semaphore::add is failed";
      logger_->error(stream.str(), Aspect::FILE_MANAGER);
    }

    thread_.reset();
  }
  catch (...)
  {
  }
}

inline void FileManager::write(
  const File& file,
  const std::string_view buffer,
  const std::int64_t offset,
  Callback&& callback) noexcept
{
  Event write_event;
  write_event.type = EventType::Write;
  write_event.fd = file.fd();
  write_event.buffer = buffer;
  write_event.offset = offset;
  write_event.callback = std::move(callback);

  add_event_to_queue(
    std::move(write_event));
}

inline int FileManager::write(
  const File& file,
  const std::string_view buffer,
  const std::int64_t offset) noexcept
{
  return call(&FileManager::write, file, buffer, offset);
}

inline void FileManager::read(
  const File& file,
  const std::string_view buffer,
  const std::int64_t offset,
  Callback&& callback) noexcept
{
  Event read_event;
  read_event.type = EventType::Read;
  read_event.fd = file.fd();
  read_event.buffer = buffer;
  read_event.offset = offset;
  read_event.callback = callback;

  add_event_to_queue(
    std::move(read_event));
}

inline int FileManager::read(
  const File& file,
  const std::string_view buffer,
  const std::int64_t offset) noexcept
{
  return call(&FileManager::read, file, buffer, offset);
}

inline int FileManager::call(
    PointerMember const pointer,
    const File& file,
    const std::string_view buffer,
    const std::int64_t offset) noexcept
{
  try
  {
    const bool is_coroutine_thread =
        userver::engine::current_task::IsTaskProcessorThread();
    if (is_coroutine_thread)
    {
      userver::engine::Promise<int> promise;
      auto future = promise.get_future();
      Callback callback(
        [promise = std::move(promise)] (const int result) mutable {
          try
          {
            promise.set_value(result);
          }
          catch (...)
          {
          }
      });

      (this->*pointer)(file, buffer, offset, std::move(callback));
      return future.get();
    }
    else
    {
      std::promise<int> promise;
      auto future = promise.get_future();
      Callback callback(
        [promise = std::move(promise)] (const int result) mutable {
          try
          {
            promise.set_value(result);
          }
          catch (...)
          {
          }
      });

      (this->*pointer)(file, buffer, offset, std::move(callback));
      return future.get();
    }
  }
  catch (const eh::Exception& exc)
  {
    try
    {
      std::ostringstream stream;
      stream << FNS
             << exc.what();
      logger_->error(stream.str(), Aspect::FILE_MANAGER);
    }
    catch (...)
    {
    }
  }
  catch (...)
  {
    try
    {
      std::ostringstream stream;
      stream << FNS
             << "Unknown error";
      logger_->error(stream.str(), Aspect::FILE_MANAGER);
    }
    catch (...)
    {
    }
  }

  return -ECANCELED;
}

inline void FileManager::add_event_to_queue(Event&& event) noexcept
{
  try
  {
    if (!event_queue_->emplace(std::move(event)))
    {
      std::stringstream stream;
      stream << FNS
             << "event_queue size limit is reached";
      logger_->error(stream.str(), Aspect::FILE_MANAGER);

      try
      {
        event.callback(-ECANCELED);
      }
      catch (...)
      {
      }
      return;
    }
    assert(semaphore_->add());
  }
  catch (const eh::Exception& exc)
  {
    try
    {
      event.callback(-ECANCELED);

      std::ostringstream stream;
      stream << FNS
             << exc.what();
      logger_->error(stream.str(), Aspect::FILE_MANAGER);
    }
    catch (...)
    {
    }
  }
  catch (...)
  {
    try
    {
      event.callback(-ECANCELED);

      std::ostringstream stream;
      stream << FNS
             << "Unknow error";
      logger_->error(stream.str(), Aspect::FILE_MANAGER);
    }
    catch (...)
    {
    }
  }
}

inline void FileManager::run(
  const SemaphorePtr& semaphore,
  IoUringPtr&& uring) noexcept
{
  io_uring_cqe* cqe = nullptr;
  bool is_stopped = false;
  bool is_semaphore_set = true;
  std::size_t number_remain_operaions = 0;
  while(!is_stopped || number_remain_operaions > 0)
  {
    if (!is_semaphore_set)
    {
      is_semaphore_set = create_semaphore_event(
        semaphore_->fd(),
        uring->get());
    }

    const auto result = io_uring_wait_cqe(uring->get(), &cqe);
    if (result != 0)
    {
      try
      {
        const auto error = -result;
        std::ostringstream stream;
        stream << FNS
               << "io_uring_wait_cqe is failed, reason=[code="
               << error
               << ", message="
               << Utils::safe_strerror(error)
               << "]";
        logger_->error(stream.str(), Aspect::FILE_MANAGER);
      }
      catch (...)
      {
      }
      continue;
    }

    std::unique_ptr<UserData> user_data;
    user_data.reset(static_cast<UserData*>(io_uring_cqe_get_data(cqe)));
    if (!user_data)
    {
      io_uring_cqe_seen(uring->get(), cqe);
      try
      {
        std::ostringstream stream;
        stream << FNS
               << "user_data is null";
        logger_->error(stream.str(), Aspect::FILE_MANAGER);
      }
      catch (...)
      {
      }
      continue;
    }

    switch (user_data->type)
    {
      case CompletionType::Semaphore:
      {
        is_semaphore_set = false;
        on_semaphore_ready(uring->get(), number_remain_operaions, is_stopped);
        break;
      }
      case CompletionType::Read:
      {
        number_remain_operaions -= 1;
        on_read_ready(cqe->res, std::move(user_data->callback));
        break;
      }
      case CompletionType::Write:
      {
        number_remain_operaions -= 1;
        on_write_ready(cqe->res, std::move(user_data->callback));
        break;
      }
    }

    io_uring_cqe_seen(uring->get(), cqe);
  }
}

inline void FileManager::on_semaphore_ready(
  io_uring* const uring,
  std::size_t& number_remain_operaions,
  bool& is_stopped) noexcept
{
  const std::uint32_t sq_limit = io_uring_sq_space_left(uring) - 1;
  const std::uint32_t cq_limit = max_size_cq_queue_ - number_remain_operaions - 1;
  const std::uint32_t max_queue_elements = std::min(sq_limit, cq_limit);
  std::uint32_t count = 1 + semaphore_->try_consume(max_queue_elements);
  while (count > 0)
  {
    auto data = event_queue_->pop();
    if (!data)
    {
      continue;
    }

    count -= 1;
    switch (data->type)
    {
      case EventType::Read:
      {
        const auto result = create_read_or_write_event(
          true,
          data->fd,
          data->buffer,
          data->offset,
          std::move(data->callback),
          uring);
        if (result)
        {
          number_remain_operaions += 1;
        }
        break;
      }
      case EventType::Write:
      {
        const auto result = create_read_or_write_event(
          false,
          data->fd,
          data->buffer,
          data->offset,
          std::move(data->callback),
          uring);
        if (result)
        {
          number_remain_operaions += 1;
        }
        break;
      }
      case EventType::Close:
      {
        is_stopped = true;
        break;
      }
    }
  }
}

inline void FileManager::on_write_ready(
  const int result,
  Callback&& callback) const noexcept
{
  try
  {
    callback(result);
  }
  catch (...)
  {
  }
}

inline void FileManager::on_read_ready(
  const int result,
  Callback&& callback) const noexcept
{
  try
  {
    callback(result);
  }
  catch (...)
  {
  }
}

inline bool FileManager::create_semaphore_event(
  const int semaphore_fd,
  io_uring* const uring) noexcept
{
  try
  {
    auto* const sqe = io_uring_get_sqe(uring);
    if (!sqe)
    {
      std::stringstream stream;
      stream << FNS
             << "io_uring_get_sqe is failed, reason=SQ ring is currently full";
      logger_->error(stream.str(), Aspect::FILE_MANAGER);
      return false;
    }

    auto user_data = std::make_unique<UserData>();
    user_data->type = CompletionType::Semaphore;

    io_uring_prep_read(
      sqe,
      semaphore_fd,
      &user_data->semaphore_buffer,
      sizeof(eventfd_t),
      0);

    auto* p_user_data = user_data.get();
    io_uring_sqe_set_data(sqe, user_data.release());

    const auto result = io_uring_submit(uring);
    if (result < 0)
    {
      user_data.reset(p_user_data);
      const auto error = -result;
      std::ostringstream stream;
      stream << FNS
             << "io_uring_submit is failed, reason=[code"
             << error
             << ", message="
             << Utils::safe_strerror(error)
             << "]";
      logger_->error(stream.str(), Aspect::FILE_MANAGER);
      return false;
    }

    return true;
  }
  catch (...)
  {
  }

  return false;
}

inline bool FileManager::create_read_or_write_event(
  const bool is_read,
  const int fd,
  const std::string_view buffer,
  const std::int64_t offset,
  Callback&& callback,
  io_uring* const uring) noexcept
{
  auto* const sqe = io_uring_get_sqe(uring);
  if (!sqe)
  {
    try
    {
      callback(-ECANCELED);

      std::stringstream stream;
      stream << FNS
             << "io_uring_get_sqe is failed, reason=SQ ring is currently full";
      logger_->error(stream.str(), Aspect::FILE_MANAGER);
    }
    catch (...)
    {
    }

    return false;
  }

  try
  {
    auto user_data = std::make_unique<UserData>();
    user_data->type = is_read ? CompletionType::Read : CompletionType::Write;
    user_data->callback = callback;

    if (is_read)
    {
      io_uring_prep_read(
        sqe,
        fd,
        const_cast<char*>(buffer.data()),
        buffer.size(),
        offset);
    }
    else
    {
      io_uring_prep_write(
        sqe,
        fd,
        buffer.data(),
        buffer.size(),
        offset);
    }

    auto* p_user_data = user_data.get();
    io_uring_sqe_set_data(sqe, user_data.release());

    const auto result = io_uring_submit(uring);
    if (result > 0)
    {
      return true;
    }
    else
    {
      user_data.reset(p_user_data);

      const auto error = -result;
      std::ostringstream stream;
      stream << FNS
             << "io_uring_submit is failed, reason=[code"
             << error
             << ", message="
             << Utils::safe_strerror(error)
             << "]";
      logger_->error(stream.str(), Aspect::FILE_MANAGER);

      try
      {
        callback(-ECANCELED);
      }
      catch (...)
      {
      }

      return false;
    }
  }
  catch (const eh::Exception& exc)
  {
    try
    {
      callback(-ECANCELED);

      std::ostringstream stream;
      stream << FNS
             << exc.what();
      logger_->error(stream.str(), Aspect::FILE_MANAGER);
    }
    catch (...)
    {
    }

    return false;
  }
  catch (...)
  {
    try
    {
      callback(-ECANCELED);
    }
    catch (...)
    {
    }

    return false;
  }
}

} // namespace UServerUtils::FileManager