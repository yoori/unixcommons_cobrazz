#ifndef USERVER_LOGGER_HPP
#define USERVER_LOGGER_HPP

// USERVER
#include <userver/logging/null_logger.hpp>
#include <userver/logging/impl/logger_base.hpp>
#include <userver/logging/level.hpp>

// THIS
#include <eh/Exception.hpp>
#include <Logger/Logger.hpp>

namespace UServerUtils::Logger
{

using LoggerBase = userver::logging::impl::LoggerBase;

class Logger final : public LoggerBase
{
public:
  using Logger_var = Logging::Logger_var;
  using Level = userver::logging::Level;

private:
  using Format = userver::logging::Format;

public:
  Logger(Logging::Logger* logger);

  ~Logger() override = default;

  void Log(Level level, std::string_view msg) override;

  void Flush() override;

private:
  Logger_var logger_;
};

using LoggerPtr = std::unique_ptr<Logger>;

class LoggerScope final : private Generics::Uncopyable
{
public:
  using Logger_var = Logging::Logger_var;
  using Level = userver::logging::Level;

public:
  LoggerScope(Logging::Logger* logger);

  ~LoggerScope();

private:
  LoggerPtr logger_new_;

  userver::logging::LoggerRef logger_prev_;
};

using LoggerScopePtr = std::unique_ptr<LoggerScope>;

} // namespace UServerUtils::Logger

#endif //USERVER_LOGGER_HPP
