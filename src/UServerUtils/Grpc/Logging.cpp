// STD
#include <array>
#include <atomic>
#include <iostream>
#include <sstream>

// USERVER
#include <userver/rcu/rcu.hpp>

// GRPC
#include <grpc/impl/codegen/log.h>

// THIS
#include <Logger/StreamLogger.hpp>
#include "Logging.hpp"

namespace UServerUtils
{
namespace Grpc
{
namespace Logger
{

namespace
{
auto& get_logger_internal()
{
  static userver::rcu::Variable<Logger_var> logger(
    Logger_var(
      new Logging::OStream::Logger(
        Logging::OStream::Config(
          std::cerr,
          Logging::Logger::ERROR))));
  return logger;
}

Logger_var get_logger()
{
  return get_logger_internal().ReadCopy();
}

auto to_log_level(::gpr_log_severity severity) noexcept
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

inline auto& get_should_log_cache() noexcept
{
  static std::array<std::atomic<bool>, Logging::BaseLogger::DEBUG + 1> values{};
  return values;
}

void update_should_log_cache() noexcept
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

  std::stringstream stream;
  stream << "File="
         << args->file
         << ", line="
         << args->line
         << ", message="
         << args->message;
  auto logger = get_logger();
  logger->log(
    stream.str(),
    log_level,
    "GRPC");
}
} // namespace

Logger_var set_logger(Logger_var logger)
{
  auto ptr = get_logger_internal().StartWrite();
  logger.swap(*ptr);
  ptr.Commit();

  update_should_log_cache();

  return logger;
}

void setup_native_logging()
{
  ::gpr_set_log_function(&log_function);
}

} // namespace Logger
} // namespace Grpc
} // namespace UServerUtils