// STD
#include <sstream>

// USERVER
#include <userver/engine/future.hpp>
#include <userver/engine/task/task.hpp>

// THIS
#include <UServerUtils/FileManager/Utils.hpp>
#include <UServerUtils/RocksDB/DataBaseManager.hpp>

namespace UServerUtils::Grpc::RocksDB
{

namespace Aspect
{

const char DATA_BASE_MANAGER[] = "DATA_BASE_MANAGER";

} // namespace Aspect

DataBaseManager::DataBaseManager(
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

DataBaseManager::DataBaseManager(
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

DataBaseManager::~DataBaseManager()
{
  try
  {
    EventPtr close_event = std::make_unique<Event>(
      std::make_unique<CloseEventData>());
    while (!event_queue_->emplace(std::move(close_event)))
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    assert(semaphore_->add());
    thread_.reset();
  }
  catch (...)
  {
  }
}

std::uint32_t DataBaseManager::uring_fd() const noexcept
{
  return uring_fd_;
}

void DataBaseManager::initialize(
  IoUringPtr&& uring)
{
  if (!create_semaphore_event(semaphore_->fd(), uring->get()))
  {
    std::ostringstream stream;
    stream << FNS
           << "create_eventfd_read_event is failed";
    throw Exception(stream.str());
  }

  thread_ = std::make_unique<Thread>(
    &DataBaseManager::run,
    this,
    semaphore_,
    std::move(uring));
}

void DataBaseManager::run(
  const SemaphorePtr& semaphore,
  IoUringPtr&& uring) noexcept
{
  auto io_uring_options = std::make_unique<IOUringOptions>(uring->get());
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
               << FileManager::Utils::safe_strerror(error)
               << "]";
        logger_->error(stream.str(), Aspect::DATA_BASE_MANAGER);
      }
      catch (...)
      {
      }
      continue;
    }

    rocksdb::FilePage* const file_page =
      static_cast<rocksdb::FilePage*>(io_uring_cqe_get_data(cqe));
    io_uring_cqe_seen(uring->get(), cqe);
    if (!file_page)
    {
      try
      {
        std::ostringstream stream;
        stream << FNS
               << "file_page is null";
        logger_->error(stream.str(), Aspect::DATA_BASE_MANAGER);
      }
      catch (...)
      {
      }
      continue;
    }

    SemaphoreData* p_semaphore_data = dynamic_cast<SemaphoreData*>(file_page);
    if (p_semaphore_data)
    {
      std::unique_ptr<SemaphoreData> semaphore_data(p_semaphore_data);
      is_semaphore_set = false;
      on_semaphore_ready(
        io_uring_options.get(),
        number_remain_operaions,
        is_stopped);
    }
    else
    {
      auto handle = std::coroutine_handle<rocksdb::async_result::promise_type>::from_promise(
        *file_page->promise);
      handle.resume();
    }
  }
}

bool DataBaseManager::create_semaphore_event(
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
      logger_->error(stream.str(), Aspect::DATA_BASE_MANAGER);
      return false;
    }

    auto semaphore_data = std::make_unique<SemaphoreData>();

    io_uring_prep_read(
      sqe,
      semaphore_fd,
      &semaphore_data->semaphore_buffer,
      sizeof(eventfd_t),
      0);

    auto* p_semaphore_data = semaphore_data.get();
    io_uring_sqe_set_data(sqe, semaphore_data.release());
    const auto result = io_uring_submit(uring);
    if (result < 0)
    {
      semaphore_data.reset(p_semaphore_data);
      const auto error = -result;
      std::ostringstream stream;
      stream << FNS
             << "io_uring_submit is failed, reason=[code"
             << error
             << ", message="
             << FileManager::Utils::safe_strerror(error)
             << "]";
      logger_->error(stream.str(), Aspect::DATA_BASE_MANAGER);
      return false;
    }

    return true;
  }
  catch (...)
  {
  }

  return false;
}

void DataBaseManager::on_semaphore_ready(
  IOUringOptions* const io_uring_options,
  std::size_t& number_remain_operaions,
  bool& is_stopped) noexcept
{
  const std::uint32_t max_queue_elements = io_uring_sq_space_left(io_uring_options->ioring) - 1;
  std::uint32_t count = 1 + semaphore_->try_consume(max_queue_elements);
  while (count > 0)
  {
    auto data = event_queue_->pop();
    // Documentation does not say that semaphore performs memory order(acquire/release).
    if (!data)
      continue;

    count -= 1;
    switch ((*data)->type)
    {
      case EventType::Put:
      case EventType::Erase:
      case EventType::Get:
      case EventType::MultiGet:
      {
        number_remain_operaions += 1;
        do_async_work(
          std::move(*data),
          io_uring_options,
          &number_remain_operaions);
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

rocksdb::async_result DataBaseManager::do_async_work(
  EventPtr event,
  rocksdb::IOUringOptions* const io_uring_options,
  std::size_t* const number_remain_operaions)
{
  assert(event);
  switch (event->type)
  {
    case EventType::Get:
    {
      auto& event_data = *std::get<GetEventDataPtr>(event->data);
      assert(event_data.callback);

      const auto& key = event_data.key;
      auto& db = event_data.db;
      auto* column_family = event_data.column_family;
      auto& callback = event_data.callback;
      auto& read_options = event_data.read_options;
      read_options.io_uring_option = io_uring_options;

      {
        rocksdb::PinnableSlice value;
        auto async_result = db->get().AsyncGet(
          read_options,
          column_family,
          rocksdb::Slice(key.data(), key.size()),
          &value,
          nullptr);
        co_await async_result;

        auto status = async_result.result();
        try
        {
          callback(std::move(status), value.ToStringView());
        }
        catch (...)
        {
        }
      }

      *number_remain_operaions -= 1;
      break;
    }
    case EventType::MultiGet:
    {
      auto& event_data = *std::get<MultiGetEventDataPtr>(event->data);
      assert(event_data.callback);

      const auto& keys = event_data.keys;
      auto& db = event_data.db;
      auto& column_families = event_data.column_families;
      auto& callback = event_data.callback;
      auto& read_options = event_data.read_options;
      read_options.io_uring_option = io_uring_options;

      std::vector<rocksdb::Slice> keys_result;
      keys_result.reserve(keys.size());
      for (const auto& key : keys)
      {
        keys_result.emplace_back(key.data(), key.size());
      }

      Values values;
      auto async_result = db->get().AsyncMultiGet(
        read_options,
        column_families,
        keys_result,
        &values,
        nullptr);
      co_await async_result;

      auto statuses = async_result.results();
      try
      {
        callback(std::move(statuses), std::move(values));
      }
      catch (...)
      {
      }

      *number_remain_operaions -= 1;
      break;
    }
    case EventType::Put:
    {
      auto& event_data = *std::get<PutEventDataPtr>(event->data);
      assert(event_data.callback);

      const auto& key = event_data.key;
      const auto& value = event_data.value;
      auto& db = event_data.db;
      auto* column_family = event_data.column_family;
      auto& callback = event_data.callback;
      auto& write_options = event_data.write_options;
      write_options.io_uring_option = io_uring_options;

      auto async_result = db->get().AsyncPut(
        write_options,
        column_family,
        rocksdb::Slice(key.data(), key.size()),
        rocksdb::Slice(value.data(), value.size()));
      co_await async_result;

      auto status = async_result.result();
      try
      {
        callback(std::move(status));
      }
      catch (...)
      {
      }

      *number_remain_operaions -= 1;
      break;
    }
    case EventType::Erase:
    {
      auto& event_data = *std::get<EraseEventDataPtr>(event->data);
      assert(event_data.callback);

      const auto& key = event_data.key;
      auto& db = event_data.db;
      auto* column_family = event_data.column_family;
      auto& callback = event_data.callback;
      auto& write_options = event_data.write_options;
      write_options.io_uring_option = io_uring_options;

      auto async_result = db->get().AsyncDelete(
        write_options,
        column_family,
        rocksdb::Slice(key.data(), key.size()));
      co_await async_result;

      auto status = async_result.result();
      try
      {
        callback(std::move(status));
      }
      catch (...)
      {
      }

      *number_remain_operaions -= 1;
      break;
    }
    default:
    {
    }
  }

  co_return Status::OK();
}

void DataBaseManager::get(
  const DataBasePtr& db,
  ColumnFamilyHandle& column_family,
  const ReadOptions& read_options,
  const std::string_view key,
  GetCallback&& callback) noexcept
{
  auto data = std::make_unique<GetEventData>(
    db,
    &column_family,
    key,
    read_options,
    std::move(callback));
  EventPtr event = std::make_unique<Event>(std::move(data));
  add_event_to_queue(std::move(event));
}

DataBaseManager::Status DataBaseManager::get(
  const DataBasePtr& db,
  ColumnFamilyHandle& column_family,
  const ReadOptions& read_options,
  const std::string_view key,
  std::string& value) noexcept
{
  using Data = std::pair<Status, std::string>;

  try
  {
    const bool is_coroutine_thread =
      userver::engine::current_task::IsTaskProcessorThread();
    if (is_coroutine_thread)
    {
      userver::engine::Promise<Data> promise;
      auto future = promise.get_future();
      GetCallback callback([promise = std::move(promise)] (
        Status&& status,
        const std::string_view value) mutable {
          try
          {
            Data data(std::move(status), value);
            promise.set_value(std::move(data));
          }
          catch (...)
          {
            try
            {
              promise.set_exception(std::current_exception());
            }
            catch (...)
            {
            }
          }
        });

      get(
        db,
        column_family,
        read_options,
        key,
        std::move(callback));
      auto result = future.get();
      value = std::move(result.second);

      return result.first;
    }
    else
    {
      std::promise<Data> promise;
      auto future = promise.get_future();
      GetCallback callback([promise = std::move(promise)] (
        Status&& status,
        const std::string_view value) mutable {
          try
          {
            Data data(std::move(status), value);
            promise.set_value(std::move(data));
          }
          catch (...)
          {
            try
            {
              promise.set_exception(std::current_exception());
            }
            catch (...)
            {
            }
          }
        });

      get(
        db,
        column_family,
        read_options,
        key,
        std::move(callback));

      auto result = future.get();
      value = std::move(result.second);
      return result.first;
    }
  }
  catch (const eh::Exception& exc)
  {
    try
    {
      std::ostringstream stream;
      stream << FNS
             << exc.what();
      logger_->error(stream.str(), Aspect::DATA_BASE_MANAGER);
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
      logger_->error(stream.str(), Aspect::DATA_BASE_MANAGER);
    }
    catch (...)
    {
    }
  }

  return Status::Corruption();
}

void DataBaseManager::multi_get(
  const DataBasePtr& db,
  ColumnFamilies&& column_families,
  const ReadOptions& read_options,
  Keys&& keys,
  MultiGetCallback&& callback) noexcept
{
  if (column_families.size() != keys.size())
  {
    Statuses statuses;
    Values values;
    try
    {
      const auto size = keys.size();
      const Status status = rocksdb::Status::Aborted(
        "Size of column_families and keys should be same");
      statuses = Statuses(size, status);
      values = Values(size, std::string{});
    }
    catch (...)
    {
    }

    try
    {
      callback(std::move(statuses), std::move(values));
    }
    catch (...)
    {
    }
  }

  auto data = std::make_unique<MultiGetEventData>(
    db,
    std::move(column_families),
    std::move(keys),
    read_options,
    std::move(callback));
  EventPtr event = std::make_unique<Event>(std::move(data));
  add_event_to_queue(std::move(event));
}

DataBaseManager::Statuses DataBaseManager::multi_get(
  const DataBasePtr& db,
  ColumnFamilies&& column_families,
  const ReadOptions& read_options,
  Keys&& keys,
  Values& values) noexcept
{
  using Data = std::pair<Statuses, Values>;

  try
  {
    const bool is_coroutine_thread =
      userver::engine::current_task::IsTaskProcessorThread();
    if (is_coroutine_thread)
    {
      userver::engine::Promise<Data> promise;
      auto future = promise.get_future();
      MultiGetCallback callback([promise = std::move(promise)] (
        Statuses&& statuses,
        Values&& values) mutable {
          try
          {
            Data data(std::move(statuses), std::move(values));
            promise.set_value(std::move(data));
          }
          catch (...)
          {
            try
            {
              promise.set_exception(std::current_exception());
            }
            catch (...)
            {
            }
          }
      });

      multi_get(
        db,
        std::move(column_families),
        read_options,
        std::move(keys),
        std::move(callback));
      auto result = future.get();
      values = std::move(result.second);

      return std::move(result.first);
    }
    else
    {
      std::promise<Data> promise;
      auto future = promise.get_future();
      MultiGetCallback callback([promise = std::move(promise)] (
        Statuses&& statuses,
        Values&& values) mutable {
          try
          {
            Data data(std::move(statuses), std::move(values));
            promise.set_value(std::move(data));
          }
          catch (...)
          {
            try
            {
              promise.set_exception(std::current_exception());
            }
            catch (...)
            {
            }
          }
      });

      multi_get(
        db,
        std::move(column_families),
        read_options,
        std::move(keys),
        std::move(callback));

      auto result = future.get();
      values = std::move(result.second);
      return std::move(result.first);
    }
  }
  catch (const eh::Exception& exc)
  {
    try
    {
      std::ostringstream stream;
      stream << FNS
             << exc.what();
      logger_->error(stream.str(), Aspect::DATA_BASE_MANAGER);
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
      logger_->error(stream.str(), Aspect::DATA_BASE_MANAGER);
    }
    catch (...)
    {
    }
  }

  return Statuses(keys.size(), Status::Corruption());
}

void DataBaseManager::put(
  const DataBasePtr& db,
  ColumnFamilyHandle& column_family,
  const WriteOptions& write_options,
  const std::string_view key,
  const std::string_view value,
  PutCallback&& callback) noexcept
{
  auto data = std::make_unique<PutEventData>(
    db,
    &column_family,
    key,
    value,
    write_options,
    std::move(callback));
  EventPtr event = std::make_unique<Event>(std::move(data));
  add_event_to_queue(std::move(event));
}

DataBaseManager::Status DataBaseManager::put(
  const DataBasePtr& db,
  ColumnFamilyHandle& column_family,
  const WriteOptions& write_options,
  const std::string_view key,
  const std::string_view value) noexcept
{
  try
  {
    const bool is_coroutine_thread =
      userver::engine::current_task::IsTaskProcessorThread();
    if (is_coroutine_thread)
    {
      userver::engine::Promise<Status> promise;
      auto future = promise.get_future();
      PutCallback callback([promise = std::move(promise)] (
        Status&& status) mutable {
          try
          {
            promise.set_value(std::move(status));
          }
          catch (...)
          {
          }
      });

      put(
        db,
        column_family,
        write_options,
        key,
        value,
        std::move(callback));

      return future.get();
    }
    else
    {
      std::promise<Status> promise;
      auto future = promise.get_future();
      PutCallback callback([promise = std::move(promise)] (
        Status&& status) mutable {
          try
          {
            promise.set_value(std::move(status));
          }
          catch (...)
          {
          }
      });

      put(
        db,
        column_family,
        write_options,
        key,
        value,
        std::move(callback));

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
      logger_->error(stream.str(), Aspect::DATA_BASE_MANAGER);
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
      logger_->error(stream.str(), Aspect::DATA_BASE_MANAGER);
    }
    catch (...)
    {
    }
  }

  return Status::Corruption();
}

void DataBaseManager::erase(
  const DataBasePtr& db,
  ColumnFamilyHandle& column_family,
  const WriteOptions& write_options,
  const std::string_view key,
  EraseCallback&& callback) noexcept
{
  auto data = std::make_unique<EraseEventData>(
    db,
    &column_family,
    key,
    write_options,
    std::move(callback));
  EventPtr event = std::make_unique<Event>(std::move(data));
  add_event_to_queue(std::move(event));
}

DataBaseManager::Status DataBaseManager::erase(
  const DataBasePtr& db,
  ColumnFamilyHandle& column_family,
  const WriteOptions& write_options,
  const std::string_view key) noexcept
{
  try
  {
    const bool is_coroutine_thread =
      userver::engine::current_task::IsTaskProcessorThread();
    if (is_coroutine_thread)
    {
      userver::engine::Promise<Status> promise;
      auto future = promise.get_future();
      EraseCallback callback([promise = std::move(promise)] (
        Status&& status) mutable {
          try
          {
            promise.set_value(status);
          }
          catch (...)
          {
          }
      });

      erase(
        db,
        column_family,
        write_options,
        key,
        std::move(callback));

      return future.get();
    }
    else
    {
      std::promise<Status> promise;
      auto future = promise.get_future();
      EraseCallback callback([promise = std::move(promise)] (
        Status&& status) mutable {
          try
          {
            promise.set_value(status);
          }
          catch (...)
          {
          }
      });

      erase(
        db,
        column_family,
        write_options,
        key,
        std::move(callback));

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
      logger_->error(stream.str(), Aspect::DATA_BASE_MANAGER);
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
      logger_->error(stream.str(), Aspect::DATA_BASE_MANAGER);
    }
    catch (...)
    {
    }
  }

  return Status::Corruption();
}

void DataBaseManager::add_event_to_queue(EventPtr&& event) noexcept
{
  try
  {
    if (!event_queue_->emplace(std::move(event))) {
      std::stringstream stream;
      stream << FNS
             << "event_queue size limit is reached";
      logger_->error(stream.str(), Aspect::DATA_BASE_MANAGER);

      set_error(
        std::move(event),
        "Event_queue size limit is reached");

      return;
    }

    assert(semaphore_->add());
  }
  catch (const eh::Exception& exc)
  {
    try
    {
      set_error(std::move(event), exc.what());

      std::ostringstream stream;
      stream << FNS
             << exc.what();
      logger_->error(stream.str(), Aspect::DATA_BASE_MANAGER);
    }
    catch (...)
    {
    }
  }
  catch (...)
  {
    try
    {
      set_error(std::move(event), "Unknown error");

      std::ostringstream stream;
      stream << FNS
             << "Unknow error";
      logger_->error(stream.str(), Aspect::DATA_BASE_MANAGER);
    }
    catch (...)
    {
    }
  }
}

void DataBaseManager::set_error(
  EventPtr&& event,
  const std::string& error_message) noexcept
{
  try
  {
    switch (event->type)
    {
      case EventType::Get:
      {
        auto& data = *std::get<GetEventDataPtr>(event->data);
        data.callback(rocksdb::Status::Aborted(error_message), {});
        break;
      }
      case EventType::MultiGet:
      {
        auto& data = *std::get<MultiGetEventDataPtr>(event->data);
        Statuses statuses;
        Values values;
        try
        {
          const auto size = data.keys.size();
          const Status status = rocksdb::Status::Aborted(error_message);
          statuses = Statuses(size, status);
          values = Values(size, std::string{});
        }
        catch (...)
        {
        }

        data.callback(std::move(statuses), std::move(values));
        break;
      }
      case EventType::Put:
      {
        auto& data = *std::get<PutEventDataPtr>(event->data);
        data.callback(rocksdb::Status::Aborted(error_message));
        break;
      }
      case EventType::Erase:
      {
        auto& data = *std::get<EraseEventDataPtr>(event->data);
        data.callback(rocksdb::Status::Aborted(error_message));
        break;
      }
      default:
      {
      }
    }
  }
  catch (...)
  {
  }
}

} // namespace UServerUtils::Grpc::RocksDB