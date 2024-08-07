// STD
#include <array>
#include <atomic>
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <sstream>

// GRPC
#include <grpc/impl/codegen/log.h>

// THIS
#include <Generics/Uncopyable.hpp>
#include <Logger/StreamLogger.hpp>
#include <UServerUtils/Grpc/Common/Logging.hpp>

namespace UServerUtils::Grpc::Common::Logger
{

namespace internal
{

class LoggerStorage final
  : protected Generics::Uncopyable
{
public:
  explicit LoggerStorage()
    : logger_(new Logging::OStream::Logger(
        Logging::OStream::Config(
          std::cerr,
          Logging::Logger::ERROR)))
  {
  }

  Logger_var get() const
  {
    std::shared_lock lock(mutex_);
    return logger_;
  }

  void set(Logging::Logger* logger)
  {
    std::unique_lock lock(mutex_);
    logger_ = Logging::Logger_var(
      ReferenceCounting::add_ref(logger));
  }

private:
  Logger_var logger_;

  mutable std::shared_mutex mutex_;
};

inline LoggerStorage& get_logger_storage()
{
  static LoggerStorage logger_storage;
  return logger_storage;
}

inline Logger_var get_logger()
{
  return get_logger_storage().get();
}

inline Logger_var set_logger(Logging::Logger* logger)
{
  auto logger_old = get_logger_storage().get();
  get_logger_storage().set(logger);
  return logger_old;
}

inline auto to_log_level(::gpr_log_severity severity) noexcept
{
  switch (severity)
  {
    case ::GPR_LOG_SEVERITY_DEBUG:
      return Logging::BaseLogger::DEBUG;
    case ::GPR_LOG_SEVERITY_INFO:
      return Logging::BaseLogger::INFO;
    case ::GPR_LOG_SEVERITY_ERROR:
      [[fallthrough]];
    default:
      return Logging::BaseLogger::ERROR;
  }
}

inline auto to_grpc_log_severity(const unsigned long saverity) noexcept
{
  if (saverity >= Logging::BaseLogger::Severity::DEBUG)
  {
    return ::GPR_LOG_SEVERITY_DEBUG;
  }
  else if (saverity >= Logging::BaseLogger::Severity::WARNING)
  {
    return ::GPR_LOG_SEVERITY_INFO;
  }
  else
  {
    return ::GPR_LOG_SEVERITY_ERROR;
  }
}

inline auto& get_should_log_cache() noexcept
{
  static std::array<std::atomic<bool>, Logging::BaseLogger::TRACE> values{};
  return values;
}

inline void update_should_log_cache() noexcept
{
  const auto log_level = get_logger()->log_level();
  auto& cache = get_should_log_cache();
  const auto size = cache.size();
  for (std::size_t i = 0; i < size; ++i)
  {
    cache[i].store(false, std::memory_order_relaxed);
  }

  for (std::size_t i = 0; i <= log_level; ++i)
  {
    cache[i].store(true, std::memory_order_relaxed);
  }
}

void log_function(::gpr_log_func_args* args) noexcept
{
  if (!args)
    return;

  const auto log_level = to_log_level(args->severity);
  if (!get_should_log_cache()[log_level].load(std::memory_order_relaxed))
    return;

  try
  {
    std::stringstream stream;
    stream << "File="
           << args->file
           << ", line="
           << args->line
           << ", message="
           << args->message;
    auto logger = get_logger();
    logger->log(stream.str(), log_level, "GRPC");
  }
  catch (...)
  {
  }
}

inline void setup_native_logging()
{
  ::gpr_set_log_function(&log_function);
}

std::mutex mutex_native_log;

} // namespace internal

Logger_var set_logger(Logging::Logger* logger)
{
  std::lock_guard lock(internal::mutex_native_log);
  ::gpr_set_log_verbosity(internal::to_grpc_log_severity(logger->log_level()));
  auto old_logger = internal::set_logger(logger);
  internal::update_should_log_cache();
  internal::setup_native_logging();
  return old_logger;
}

} // namespace UServerUtils::Grpc::Common::Logger