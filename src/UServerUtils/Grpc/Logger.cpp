// USERVER
#include <userver/logging/log.hpp>

// THIS
#include <UServerUtils/Grpc/Logger.hpp>

namespace UServerUtils::Grpc::Logger
{

namespace Aspect
{

const char USERVER[] = "USERVER";

} // namespace Aspect

Logger::Logger(Logging::Logger* logger)
  : LoggerBase(Format::kRaw),
    logger_(ReferenceCounting::add_ref(logger))
{
  Level userver_level = Level::kError;
  const auto level = logger_->log_level();
  switch (level)
  {
  case Logging::BaseLogger::EMERGENCY:
    userver_level = Level::kCritical;
    break;
  case Logging::BaseLogger::ALERT:
    userver_level = Level::kCritical;
    break;
  case Logging::BaseLogger::CRITICAL:
    userver_level = Level::kCritical;
    break;
  case Logging::BaseLogger::ERROR:
    userver_level = Level::kError;
    break;
  case Logging::BaseLogger::WARNING:
    userver_level = Level::kWarning;
    break;
  case Logging::BaseLogger::NOTICE:
    userver_level = Level::kWarning;
    break;
  case Logging::BaseLogger::INFO:
    userver_level = Level::kInfo;
    break;
  case Logging::BaseLogger::DEBUG:
    userver_level = Level::kDebug;
    break;
  case Logging::BaseLogger::TRACE:
    userver_level = Level::kTrace;
    break;
  }

  LoggerBase::SetLevel(userver_level);
}

void Logger::Log(
  Level level,
  std::string_view msg) const
{
  if (ShouldLog(level))
  {
    Logging::BaseLogger::Severity severity = Logging::BaseLogger::Severity::INFO;
    switch (level)
    {
    case  Level::kCritical:
      severity = Logging::BaseLogger::CRITICAL;
      break;
    case  Level::kError:
      severity = Logging::BaseLogger::ERROR;
      break;
    case  Level::kWarning:
      severity = Logging::BaseLogger::WARNING;
      break;
    case  Level::kInfo:
      severity = Logging::BaseLogger::INFO;
      break;
    case  Level::kDebug:
      severity = Logging::BaseLogger::DEBUG;
      break;
    case  Level::kTrace:
      severity = Logging::BaseLogger::TRACE;
      break;
    case  Level::kNone:
      severity = Logging::BaseLogger::EMERGENCY;
      break;
    }

    logger_->log(
      String::SubString(msg.data(), msg.size()),
      severity,
      Aspect::USERVER);
  }
}

void Logger::Flush() const
{
}

LoggerScope::LoggerScope(Logging::Logger* logger)
  : logger_new_(std::make_unique<Logger>(logger)),
    logger_prev_(userver::logging::impl::DefaultLoggerRef())
{
  userver::logging::impl::SetDefaultLoggerRef(*logger_new_);
}

LoggerScope::~LoggerScope()
{
  userver::logging::impl::SetDefaultLoggerRef(logger_prev_);
}

} // namespace UServerUtils::Grpc::Logger