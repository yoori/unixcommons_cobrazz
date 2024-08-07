#ifndef USERVER_ROCKSDB_DATABASEMANAGER_HPP
#define USERVER_ROCKSDB_DATABASEMANAGER_HPP

// Rocksdb
#include <rocksdb/async_result.h>

// POSIX
#include <liburing.h>
#include <sys/eventfd.h>

// BOOST
#include <boost/thread/scoped_thread.hpp>

// STD
#include <functional>
#include <memory>
#include <thread>

// THIS
#include <eh/Exception.hpp>
#include <Generics/Uncopyable.hpp>
#include <Logger/Logger.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <UServerUtils/FileManager/IoUring.hpp>
#include <UServerUtils/FileManager/Semaphore.hpp>
#include <UServerUtils/Grpc/Common/QueueAtomic.hpp>
#include <UServerUtils/RocksDB/Config.hpp>
#include <UServerUtils/RocksDB/DataBase.hpp>
#include <UServerUtils/Function.hpp>

namespace UServerUtils::Grpc::RocksDB
{

class DataBaseManager final : private Generics::Uncopyable
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using DB = rocksdb::DB;
  using ColumnFamilyHandle = rocksdb::ColumnFamilyHandle;
  using ColumnFamilies = std::vector<ColumnFamilyHandle*>;
  using Status = rocksdb::Status;
  using ReadOptions = rocksdb::ReadOptions;
  using GetCallback = Utils::Function<void(Status&&, const std::string_view)>;
  using Keys = std::vector<std::string_view>;
  using Values = std::vector<std::string>;
  using Statuses = std::vector<Status>;
  using MultiGetCallback = Utils::Function<void(Statuses&&, Values&&)>;
  using WriteOptions = rocksdb::WriteOptions;
  using PutCallback = Utils::Function<void(Status&&)>;
  using EraseCallback = Utils::Function<void(Status&&)>;
  using DataBasePtr = std::shared_ptr<DataBase>;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

private:
  using Semaphore = FileManager::Semaphore;
  using IoUring = FileManager::IoUring;
  using IoUringPtr = std::unique_ptr<FileManager::IoUring>;
  using IOUringOptions = rocksdb::IOUringOptions;
  using Thread = boost::scoped_thread<boost::join_if_joinable, std::thread>;
  using ThreadPtr = std::unique_ptr<Thread>;

private:
  enum class EventType
  {
    Close = 0,
    Put,
    Erase,
    Get,
    MultiGet,
  };

  struct CloseEventData final
  {
  };

  struct PutEventData final
  {
    explicit PutEventData(
      const DataBasePtr& db,
      ColumnFamilyHandle* column_family,
      const std::string_view key,
      const std::string_view value,
      const WriteOptions& write_options,
      PutCallback&& callback)
      : db(db),
        column_family(column_family),
        key(key),
        value(value),
        write_options(write_options),
        callback(std::move(callback))
    {
    }

    ~PutEventData() = default;

    DataBasePtr db;
    ColumnFamilyHandle* column_family = nullptr;
    std::string_view key;
    std::string_view value;
    WriteOptions write_options;
    PutCallback callback;
  };

  struct EraseEventData final
  {
    explicit EraseEventData(
      const DataBasePtr& db,
      ColumnFamilyHandle* column_family,
      const std::string_view key,
      const WriteOptions& write_options,
      EraseCallback&& callback)
      : db(db),
        column_family(column_family),
        key(key),
        write_options(write_options),
        callback(std::move(callback))
    {
    }

    ~EraseEventData() = default;

    DataBasePtr db;
    ColumnFamilyHandle* column_family = nullptr;
    std::string_view key;
    WriteOptions write_options;
    EraseCallback callback;
  };

  struct GetEventData final
  {
    explicit GetEventData(
      const DataBasePtr& db,
      ColumnFamilyHandle* column_family,
      const std::string_view key,
      const ReadOptions& read_options,
      GetCallback&& callback)
      : db(db),
        column_family(column_family),
        key(key),
        read_options(read_options),
        callback(std::move(callback))
    {
    }

    ~GetEventData() = default;

    DataBasePtr db;
    ColumnFamilyHandle* column_family = nullptr;
    std::string_view key;
    ReadOptions read_options;
    GetCallback callback;
  };

  struct MultiGetEventData final
  {
    explicit MultiGetEventData(
      const DataBasePtr& db,
      ColumnFamilies&& column_families,
      Keys&& keys,
      const ReadOptions& read_options,
      MultiGetCallback&& callback)
      : db(db),
        column_families(std::move(column_families)),
        keys(std::move(keys)),
        read_options(read_options),
        callback(std::move(callback))
    {
    }

    ~MultiGetEventData() = default;

    DataBasePtr db;
    ColumnFamilies column_families;
    Keys keys;
    ReadOptions read_options;
    MultiGetCallback callback;
  };

  using CloseEventDataPtr = std::unique_ptr<CloseEventData>;
  using GetEventDataPtr = std::unique_ptr<GetEventData>;
  using MultiGetEventDataPtr = std::unique_ptr<MultiGetEventData>;
  using PutEventDataPtr = std::unique_ptr<PutEventData>;
  using EraseEventDataPtr = std::unique_ptr<EraseEventData>;

  struct Event final
  {
    using Data = std::variant<
      CloseEventDataPtr,
      GetEventDataPtr,
      MultiGetEventDataPtr,
      PutEventDataPtr,
      EraseEventDataPtr>;

    explicit Event(GetEventDataPtr&& data)
      : type(EventType::Get),
        data(std::move(data))
    {
    }

    explicit Event(MultiGetEventDataPtr&& data)
      : type(EventType::MultiGet),
        data(std::move(data))
    {
    }

    Event(PutEventDataPtr&& data)
      : type(EventType::Put),
        data(std::move(data))
    {
    }

    Event(EraseEventDataPtr&& data)
      : type(EventType::Erase),
        data(std::move(data))
    {
    }

    Event(CloseEventDataPtr&& data)
      : type(EventType::Close),
        data(std::move(data))
    {
    }

    ~Event() = default;

    Event(Event&&) = default;
    Event& operator=(Event&&) = default;
    Event(const Event&) = delete;
    Event& operator=(const Event&) = delete;

    EventType type = EventType::Close;
    Data data;
  };

  struct SemaphoreData final : rocksdb::FilePage
  {
    SemaphoreData() = default;
    ~SemaphoreData() override = default;

    eventfd_t semaphore_buffer = 0;
  };

  using EventPtr = std::unique_ptr<Event>;
  using EventQueue = UServerUtils::Grpc::Common::QueueAtomic<EventPtr>;
  using EventQueuePtr = std::shared_ptr<EventQueue>;
  using SemaphorePtr = std::shared_ptr<Semaphore>;

public:
  explicit DataBaseManager(
    const Config& config,
    Logger* logger);

  explicit DataBaseManager(
    const Config& config,
    const std::uint32_t uring_fd,
    Logger* logger);

  ~DataBaseManager();

  std::uint32_t uring_fd() const noexcept;

  /**
   * You must ensure key survives callback.
   **/
  void get(
    const DataBasePtr& db,
    ColumnFamilyHandle& column_family,
    const ReadOptions& read_options,
    const std::string_view key,
    GetCallback&& callback) noexcept;

  /**
   * If call from coroutine, block coroutine.
   * Otherwise block thread.
   **/
  Status get(
    const DataBasePtr& db,
    ColumnFamilyHandle& column_family,
    const ReadOptions& read_options,
    const std::string_view key,
    std::string& value) noexcept;

  /**
   * You must ensure that keys survives callback.
   **/
  void multi_get(
    const DataBasePtr& db,
    ColumnFamilies&& column_families,
    const ReadOptions& read_options,
    Keys&& keys,
    MultiGetCallback&& callback) noexcept;

  /**
   * If call from coroutine, block coroutine.
   * Otherwise block thread.
   **/
  Statuses multi_get(
    const DataBasePtr& db,
    ColumnFamilies&& column_families,
    const ReadOptions& read_options,
    Keys&& keys,
    Values& values) noexcept;

  /**
   * You must ensure that key and value survives callback.
   * WriteOptions::disableWAL must be true (error in realisation).
   **/
  void put(
    const DataBasePtr& db,
    ColumnFamilyHandle& column_family,
    const WriteOptions& write_options,
    const std::string_view key,
    const std::string_view value,
    PutCallback&& callback) noexcept;

  /**
   * If call from coroutine, block coroutine.
   * Otherwise block thread.
   **/
  Status put(
    const DataBasePtr& db,
    ColumnFamilyHandle& column_family,
    const WriteOptions& write_options,
    const std::string_view key,
    const std::string_view value) noexcept;

  /**
   * You must ensure that key survives callback.
   * WriteOptions::disableWAL must be true (error in realisation).
   **/
  void erase(
    const DataBasePtr& db,
    ColumnFamilyHandle& column_family,
    const WriteOptions& write_options,
    const std::string_view key,
    EraseCallback&& callback) noexcept;

  /**
   * If call from coroutine, block coroutine.
   * Otherwise block thread.
   **/
  Status erase(
    const DataBasePtr& db,
    ColumnFamilyHandle& column_family,
    const WriteOptions& write_options,
    const std::string_view key) noexcept;

private:
  void initialize(
    IoUringPtr&& uring);

  void run(
    const SemaphorePtr& semaphore,
    IoUringPtr&& uring) noexcept;

  bool create_semaphore_event(
    const int semaphore_fd,
    io_uring* const uring) noexcept;

  void on_semaphore_ready(
    IOUringOptions* const io_uring_options,
    std::size_t& number_remain_operaions,
    bool& is_cansel) noexcept;

  void add_event_to_queue(
    EventPtr&& event) noexcept;

  void set_error(
    EventPtr&& event,
    const std::string& error_message) noexcept;

  rocksdb::async_result do_async_work(
    EventPtr event,
    IOUringOptions* const io_uring_options,
    std::size_t* const number_remain_operaions);

private:
  const Logger_var logger_;

  const EventQueuePtr event_queue_;

  const SemaphorePtr semaphore_;

  std::uint32_t uring_fd_ = 0;

  ThreadPtr thread_;
};

} // namespace UServerUtils::Grpc::RocksDB

#endif // USERVER_ROCKSDB_DATABASEMANAGER_HPP