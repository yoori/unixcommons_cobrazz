#ifndef USERVER_GRPC_LOGGING_HPP
#define USERVER_GRPC_LOGGING_HPP

// THIS
#include <Logger/Logger.hpp>

namespace UServerUtils
{
namespace Grpc
{
namespace Logger
{

using Logger_var = Logging::Logger_var;

Logger_var set_logger(Logger_var logger);

void setup_native_logging();

} // namespace Logging
} // namespace Grpc
} // namespace UServerUtils

#endif //USERVER_GRPC_LOGGING_HPP
