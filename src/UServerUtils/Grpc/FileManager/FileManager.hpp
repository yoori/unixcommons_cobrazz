#ifndef USERVER_FILEMANAGER_FILEMANAGER_HPP
#define USERVER_FILEMANAGER_FILEMANAGER_HPP

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
#include <UServerUtils/Grpc/FileManager/File.hpp>
#include <UServerUtils/Grpc/FileManager/IoUring.hpp>
#include <UServerUtils/Grpc/FileManager/Semaphore.hpp>
#include <UServerUtils/Grpc/Function.hpp>

namespace UServerUtils::Grpc::FileManager
{

class FileManager final : private Generics::Uncopyable
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using IoUringPtr = std::unique_ptr<IoUring>;
  using Thread = boost::scoped_thread<boost::join_if_joinable, std::thread>;
  using ThreadPtr = std::unique_ptr<Thread>;
  using Callback = UServerUtils::Grpc::Utils::Function<void(int)>;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

private:
  enum class EventType
  {
    Close = 0,
    Read,
    Write
  };

  struct Event final
  {
    Event() = default;
    ~Event() = default;
    Event(Event&&) = default;
    Event& operator=(Event&&) = default;
    Event(const Event&) = delete;
    Event& operator=(const Event&) = delete;

    EventType type = EventType::Close;
    int fd = -1;
    std::string_view buffer;
    std::int64_t offset = 0;
    Callback callback;
  };

  enum class CompletionType
  {
    Read,
    Write,
    Semaphore
  };

  struct UserData
  {
    UserData() = default;
    ~UserData() = default;

    CompletionType type = CompletionType::Semaphore;
    eventfd_t semaphore_buffer = 0;
    Callback callback;
  };

  using EventQueue = UServerUtils::Grpc::Core::Common::QueueAtomic<Event>;
  using EventQueuePtr = std::shared_ptr<EventQueue>;
  using PointerMember = void(FileManager::*) (
    const File&,
    const std::string_view,
    const std::int64_t,
    Callback&&) noexcept;

public:
  explicit FileManager(
    const Config& config,
    Logging::Logger* logger);

  /**
   * You must ensure that buffer survives callback.
   * On success, the number of written bytes pass to callback.
   * On error, -error pass to callback.
   * */
  void write(
    const File& file,
    const std::string_view buffer,
    const std::int64_t offset,
    Callback&& callback) noexcept;

  /*
   * If call from coroutine, block coroutine.
   * Otherwise block thread.
   * On success, return number of written bytes.
   * On error, return -error.
   * */
  int write(
    const File& file,
    const std::string_view buffer,
    const std::int64_t offset) noexcept;

  /**
   * You must ensure that buffer survives callback.
   * On success, the number of read bytes pass to callback.
   * On error, -error pass to callback.
   * */
  void read(
    const File& file,
    const std::string_view buffer,
    const std::int64_t offset,
    Callback&& callback) noexcept;

  /**
   * If call from coroutine, block coroutine.
   * Otherwise block thread.
   * On success, return number of read bytes.
   * On error, return -error.
   **/
  int read(
    const File& file,
    const std::string_view buffer,
    const std::int64_t offset) noexcept;

  ~FileManager();

private:
  void initialize(IoUringPtr&& uring);

  void run(
    const int semaphore_fd,
    IoUringPtr&& uring) noexcept;

  bool create_semaphore_event(
    const int semaphore_fd,
    io_uring* const uring) noexcept;

  void create_read_or_write_event(
    const bool is_read,
    const int fd,
    const std::string_view buffer,
    const std::int64_t offset,
    Callback&& callback,
    io_uring* const uring) noexcept;

  void on_semaphore_ready(
    io_uring* const uring,
    bool& is_cansel) noexcept;

  void on_write_ready(
    const int result,
    Callback&& callback) const noexcept;

  void on_read_ready(
    const int result,
    Callback&& callback) const noexcept;

  void add_event_to_queue(
    Event&& event,
    Callback&& callback) noexcept;

  int call(
    PointerMember const pointer,
    const File& file,
    const std::string_view buffer,
    const std::int64_t offset) noexcept;

private:
  Logger_var logger_;

  EventQueuePtr event_queue_;

  Semaphore semaphore_;

  ThreadPtr thread_;
};

} // namespace UServerUtils::Grpc::FileManager

#endif // USERVER_FILEMANAGER_FILEMANAGER_HPP