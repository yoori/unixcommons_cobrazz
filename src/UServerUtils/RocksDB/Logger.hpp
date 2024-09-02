#ifndef USERVER_ROCKSDB_LOGGER_HPP
#define USERVER_ROCKSDB_LOGGER_HPP

// RocksDB
#include <rocksdb/env.h>

// THIS
#include <eh/Exception.hpp>
#include <Logger/Logger.hpp>

namespace UServerUtils::Grpc::RocksDB
{

class Logger final
  : public rocksdb::Logger,
    private Generics::Uncopyable
{
public:
  using Logger_var = Logging::Logger_var;

public:
  explicit Logger(Logging::Logger* logger);

  ~Logger() override;

  void Logv(
    const char* format,
    va_list ap) override;

  void Logv(
    const rocksdb::InfoLogLevel log_level,
    const char* format,
    va_list ap) override;

private:
  rocksdb::InfoLogLevel convert_to_rocksdb_level(
    const Logging::BaseLogger::Severity cobrazz_level) const noexcept;

  Logging::BaseLogger::Severity convert_to_cobrazz_level(
    const rocksdb::InfoLogLevel rocksdb_level) const noexcept;

  void print(
    const rocksdb::InfoLogLevel log_level,
    const char* format,
    va_list ap);

private:
  const Logger_var logger_;

  char buffer_[500];
};

} // namespace UServerUtils::Grpc::RocksDB

#include <UServerUtils/RocksDB/Logger.ipp>

#endif // USERVER_ROCKSDB_LOGGER_HPP