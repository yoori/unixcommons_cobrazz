// POSIX
#include <stdio.h>

// STD
#include <sstream>

// THIS
#include <UServerUtils/Grpc/RocksDB/Logger.hpp>

namespace UServerUtils::Grpc::RocksDB
{

namespace Aspect
{

const char LOGGER[] = "LOGGER";

} // namespace Aspect

Logger::Logger(Logging::Logger* logger)
  : rocksdb::Logger(convert_to_rocksdb_level(
      static_cast<Logging::BaseLogger::Severity>(
        logger->log_level()))),
    logger_(ReferenceCounting::add_ref(logger))
{
}

Logger::~Logger()
{
  Close();
}

rocksdb::InfoLogLevel Logger::convert_to_rocksdb_level(
  const Logging::BaseLogger::Severity cobrazz_level) const noexcept
{
  switch (cobrazz_level)
  {
    case Logging::BaseLogger::EMERGENCY:
      return rocksdb::InfoLogLevel::FATAL_LEVEL;
    case Logging::BaseLogger::ALERT:
      return rocksdb::InfoLogLevel::FATAL_LEVEL;
    case Logging::BaseLogger::CRITICAL:
      return rocksdb::InfoLogLevel::FATAL_LEVEL;
    case Logging::BaseLogger::ERROR:
      return rocksdb::InfoLogLevel::ERROR_LEVEL;
    case Logging::BaseLogger::WARNING:
      return rocksdb::InfoLogLevel::WARN_LEVEL;
    case Logging::BaseLogger::NOTICE:
      return rocksdb::InfoLogLevel::INFO_LEVEL;
    case Logging::BaseLogger::INFO:
      return rocksdb::InfoLogLevel::INFO_LEVEL;
    case Logging::BaseLogger::DEBUG:
      return rocksdb::InfoLogLevel::DEBUG_LEVEL;
    case Logging::BaseLogger::TRACE:
      return rocksdb::InfoLogLevel::DEBUG_LEVEL;
    default:
      return rocksdb::InfoLogLevel::INFO_LEVEL;
  }
}

Logging::BaseLogger::Severity Logger::convert_to_cobrazz_level(
  const rocksdb::InfoLogLevel rocksdb_level) const noexcept
{
  switch (rocksdb_level)
  {
    case rocksdb::InfoLogLevel::FATAL_LEVEL:
      return Logging::BaseLogger::EMERGENCY;
    case rocksdb::InfoLogLevel::ERROR_LEVEL:
      return Logging::BaseLogger::ERROR;
    case rocksdb::InfoLogLevel::WARN_LEVEL:
      return Logging::BaseLogger::WARNING;
    case rocksdb::InfoLogLevel::INFO_LEVEL:
      return Logging::BaseLogger::INFO;
    case rocksdb::InfoLogLevel::DEBUG_LEVEL:
      return Logging::BaseLogger::DEBUG;
    default:
      return Logging::BaseLogger::INFO;
  }
}

void Logger::Logv(
  const char* format,
  va_list ap)
{
  print(rocksdb::InfoLogLevel::INFO_LEVEL, format, ap);
}

void Logger::Logv(
  const rocksdb::InfoLogLevel log_level,
  const char* format,
  va_list ap)
{
  print(log_level, format, ap);
}

void Logger::print(
  const rocksdb::InfoLogLevel log_level,
  const char* format,
  va_list ap)
{
  const int n = vsnprintf(
    buffer_,
    sizeof(buffer_),
    format,
    ap);
  if (n < 0)
  {
    std::ostringstream stream;
    stream << FNS
           << " vsnprintf is failed";
    logger_->error(stream.str(), Aspect::LOGGER);
    return;
  }

  logger_->log(
    String::SubString(buffer_, std::min(n, static_cast<int>(sizeof(buffer_)))),
    convert_to_cobrazz_level(log_level),
    Aspect::LOGGER);
}

} // namespace UServerUtils::Grpc::RocksDB