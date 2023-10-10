// STD
#include <future>
#include <iostream>

// USERVER
#include <userver/engine/future.hpp>
#include <userver/engine/task/task.hpp>

// THIS
#include <Generics/Function.hpp>
#include <UServerUtils/Grpc/FileManager/FileManager.hpp>
#include <UServerUtils/Grpc/FileManager/Utils.hpp>

namespace UServerUtils::Grpc::FileManager
{

namespace Aspect
{

const char FILE_MANAGER[] = "FILE_MANAGER";

} // namespace Aspect

FileManager::FileManager(
  const Config& config,
  Logger* logger)
  : logger_(ReferenceCounting::add_ref(logger)),
    event_queue_(std::make_shared<EventQueue>(
      config.event_queue_max_size)),
    semaphore_(false, 0)
{
  auto uring = std::make_unique<IoUring>(config);
  initialize(std::move(uring));
}

void FileManager::initialize(IoUringPtr&& uring)
{
  if (!create_semaphore_event(semaphore_.fd(), uring->get()))
  {
    std::ostringstream stream;
    stream << FNS
           << "create_eventfd_read_event is failed";
    throw Exception(stream.str());
  }

  thread_ = std::make_unique<Thread>(
    &FileManager::run,
    this,
    semaphore_.fd(),
    std::move(uring));
}

FileManager::~FileManager()
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
    while (!semaphore_.add() && i < max_attempts)
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

void FileManager::write(
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
  write_event.callback = callback;

  add_event_to_queue(
    std::move(write_event),
    std::move(callback));
}

int FileManager::write(
  const File& file,
  const std::string_view buffer,
  const std::int64_t offset) noexcept
{
  return call(&FileManager::write, file, buffer, offset);
}

void FileManager::read(
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
    std::move(read_event),
    std::move(callback));
}

int FileManager::read(
  const File& file,
  const std::string_view buffer,
  const std::int64_t offset) noexcept
{
  return call(&FileManager::read, file, buffer, offset);
}

int FileManager::call(
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

void FileManager::add_event_to_queue(
  Event&& event,
  Callback&& callback) noexcept
{
  try
  {
    if (!event_queue_->emplace(std::move(event))) {
      std::stringstream stream;
      stream << FNS
             << "event_queue size limit is reached";
      logger_->error(stream.str(), Aspect::FILE_MANAGER);

      try
      {
        callback(-ECANCELED);
      }
      catch (...)
      {
      }

      return;
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
    return;
  }
  catch (...)
  {
    try
    {
      callback(-ECANCELED);

      std::ostringstream stream;
      stream << FNS
             << "Unknow error";
      logger_->error(stream.str(), Aspect::FILE_MANAGER);
    }
    catch (...)
    {
    }
    return;
  }

  semaphore_.add();
}

void FileManager::run(
  const int semaphore_fd,
  IoUringPtr&& uring) noexcept
{
  io_uring_cqe* cqe = nullptr;
  bool is_stopped = false;
  bool is_semaphore_set = true;
  while(!is_stopped)
  {
    if (!is_semaphore_set)
    {
      is_semaphore_set = create_semaphore_event(
        semaphore_fd,
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
    io_uring_cqe_seen(uring->get(), cqe);
    if (!user_data)
    {
      try
      {
        std::ostringstream stream;
        stream << FNS
               << "user_data is null";
        logger_->error(stream.str(), Aspect::FILE_MANAGER);
        continue;
      }
      catch (...)
      {
      }
    }

    switch (user_data->type)
    {
      case CompletionType::Semaphore:
      {
        is_semaphore_set = false;
        on_semaphore_ready(uring->get(), is_stopped);
        break;
      }
      case CompletionType::Read:
      {
        on_read_ready(cqe->res, std::move(user_data->callback));
        break;
      }
      case CompletionType::Write:
      {
        on_write_ready(cqe->res, std::move(user_data->callback));
        break;
      }
    }
  }
}

void FileManager::on_semaphore_ready(
  io_uring* const uring,
  bool& is_cansel) noexcept
{
  try
  {
    auto data = event_queue_->pop();
    if (!data)
      return;

    switch (data->type)
    {
      case EventType::Read:
      {
        create_read_or_write_event(
          true,
          data->fd,
          data->buffer,
          data->offset,
          std::move(data->callback),
          uring);
        break;
      }
      case EventType::Write:
      {
        create_read_or_write_event(
          false,
          data->fd,
          data->buffer,
          data->offset,
          std::move(data->callback),
          uring);
        break;
      }
      case EventType::Close:
      {
        is_cansel = true;
        break;
      }
    }
  }
  catch (...)
  {
  }
}

void FileManager::on_write_ready(
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

void FileManager::on_read_ready(
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

bool FileManager::create_semaphore_event(
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

void FileManager::create_read_or_write_event(
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
    return;
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

      try
      {
        callback(-ECANCELED);
      }
      catch (...)
      {
      }

      return;
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
    return;
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
    return;
  }
}

} // namespace UServerUtils::Grpc::FileManager