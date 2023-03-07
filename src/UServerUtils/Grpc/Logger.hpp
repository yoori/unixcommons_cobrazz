#ifndef USERVER_GRPC_LOGGER_HPP
#define USERVER_GRPC_LOGGER_HPP

// USERVER
#include <userver/logging/null_logger.hpp>
#include <userver/logging/impl/logger_base.hpp>
#include <userver/logging/level.hpp>

// THIS
#include <eh/Exception.hpp>
#include <Logger/Logger.hpp>

namespace UServerUtils::Grpc::Logger
{

using LoggerBase = userver::logging::impl::LoggerBase;

class Logger : public LoggerBase
{
public:
  using Logger_var = Logging::Logger_var;
  using Level = userver::logging::Level;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

private:
  using Format = userver::logging::Format;

public:
  Logger(const Logger_var& logger);

  ~Logger() override = default;

  void Log(Level level, std::string_view msg) const override;

  void Flush() const override;

private:
  Logger_var logger_;
};

using LoggerPtr = userver::logging::LoggerPtr;

} // namespace UServerUtils::Grpc::Logger

#endif //USERVER_GRPC_LOGGER_HPP
