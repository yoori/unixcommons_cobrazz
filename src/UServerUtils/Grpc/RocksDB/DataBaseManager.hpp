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
#include <UServerUtils/Grpc/Core/Common/QueueAtomic.hpp>
#include <UServerUtils/Grpc/FileManager/IoUring.hpp>
#include <UServerUtils/Grpc/FileManager/Semaphore.hpp>
#include <UServerUtils/Grpc/RocksDB/Config.hpp>
#include <UServerUtils/Grpc/RocksDB/DataBase.hpp>
#include <UServerUtils/Grpc/Function.hpp>

namespace UServerUtils::Grpc::RocksDB
{

class DataBaseManager final : private Generics::Uncopyable
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using IoUring = FileManager::IoUring;
  using IoUringPtr = std::unique_ptr<FileManager::IoUring>;
  using ColumnFamilyHandle = rocksdb::ColumnFamilyHandle;
  using ColumnFamilies = std::vector<ColumnFamilyHandle*>;
  using DB = rocksdb::DB;
  using Status = rocksdb::Status;
  using ReadOptions = rocksdb::ReadOptions;
  using GetCallback = UServerUtils::Grpc::Utils::Function<
    void(const Status&, const std::string_view)>;
  using Keys = std::vector<std::string_view>;
  using Values = std::vector<std::string>;
  using MultiGetCallback = UServerUtils::Grpc::Utils::Function<
    void(const Status&, Values&&)>;
  using WriteOptions = rocksdb::WriteOptions;
  using PutCallback = UServerUtils::Grpc::Utils::Function<void(const Status&)>;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

private:
  using Semaphore = FileManager::Semaphore;
  using IOUringOptions = rocksdb::IOUringOptions;
  using Thread = boost::scoped_thread<boost::join_if_joinable, std::thread>;
  using ThreadPtr = std::unique_ptr<Thread>;

private:
  enum class EventType
  {
    Close = 0,
    Put,
    Get,
    MultiGet,
  };

  struct CloseEventData final
  {
  };

  struct PutEventData final
  {
    explicit PutEventData(
      DB* db,
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

    DB* db = nullptr;
    ColumnFamilyHandle* column_family = nullptr;
    std::string_view key;
    std::string_view value;
    WriteOptions write_options;
    PutCallback callback;
  };

  struct GetEventData final
  {
    explicit GetEventData(
      DB* db,
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

    DB* db = nullptr;
    ColumnFamilyHandle* column_family = nullptr;
    std::string_view key;
    ReadOptions read_options;
    GetCallback callback;
  };

  struct MultiGetEventData final
  {
    explicit MultiGetEventData(
      DB* db,
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

    DB* db = nullptr;
    ColumnFamilies column_families;
    Keys keys;
    ReadOptions read_options;
    MultiGetCallback callback;
  };

  struct Event final
  {
    using Data = std::variant<
      CloseEventData,
      GetEventData,
      MultiGetEventData,
      PutEventData>;

    explicit Event(GetEventData&& data)
      : type(EventType::Get),
        data(std::move(data))
    {
    }

    explicit Event(MultiGetEventData&& data)
      : type(EventType::MultiGet),
        data(std::move(data))
    {
    }

    Event(PutEventData&& data)
      : type(EventType::Put),
        data(std::move(data))
    {
    }

    Event(CloseEventData&& data)
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

  using EventQueue = UServerUtils::Grpc::Core::Common::QueueAtomic<Event>;
  using EventQueuePtr = std::shared_ptr<EventQueue>;

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
   * You must ensure that db, column_family and key survives callback.
   **/
  void get(
    const DataBase& db,
    ColumnFamilyHandle& column_family,
    const ReadOptions& read_options,
    const std::string_view key,
    GetCallback&& callback) noexcept;

  /**
   * If call from coroutine, block coroutine.
   * Otherwise block thread.
   **/
  Status get(
    const DataBase& db,
    ColumnFamilyHandle& column_family,
    const ReadOptions& read_options,
    const std::string_view key,
    std::string& value) noexcept;

  /**
   * You must ensure that db, column_families and keys survives callback.
   **/
  void multi_get(
    const DataBase& db,
    ColumnFamilies&& column_families,
    const ReadOptions& read_options,
    Keys&& keys,
    MultiGetCallback&& callback) noexcept;

  /**
   * If call from coroutine, block coroutine.
   * Otherwise block thread.
   **/
  Status multi_get(
    const DataBase& db,
    ColumnFamilies&& column_families,
    const ReadOptions& read_options,
    Keys&& keys,
    Values& values) noexcept;

  /**
   * You must ensure that db, column_family, key and value survives callback.
   * WriteOptions::disableWAL must be true (error in realisation).
   **/
  void put(
    const DataBase& db,
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
    const DataBase& db,
    ColumnFamilyHandle& column_family,
    const WriteOptions& write_options,
    const std::string_view key,
    const std::string_view value) noexcept;

private:
  void initialize(
    IoUringPtr&& uring);

  void run(
    const int semaphore_fd,
    IoUringPtr&& uring) noexcept;

  bool create_semaphore_event(
    const int semaphore_fd,
    io_uring* const uring) noexcept;

  void on_semaphore_ready(
    IOUringOptions* const io_uring_options,
    bool& is_cansel) noexcept;

  void add_event_to_queue(
    Event&& event) noexcept;

  void set_error(
    Event&& event,
    const std::string& error_message) noexcept;

  rocksdb::async_result do_async_work(
    std::unique_ptr<Event>&& event,
    IOUringOptions* const io_uring_options);

private:
  Logger_var logger_;

  EventQueuePtr event_queue_;

  Semaphore semaphore_;

  std::uint32_t uring_fd_ = 0;

  ThreadPtr thread_;
};

} // namespace UServerUtils::Grpc::RocksDB

#endif // USERVER_ROCKSDB_DATABASEMANAGER_HPP