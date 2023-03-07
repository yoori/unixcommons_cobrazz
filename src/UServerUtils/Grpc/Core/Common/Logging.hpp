#ifndef GRPC_CORE_COMMON_LOGGING_H_
#define GRPC_CORE_COMMON_LOGGING_H_

// THIS
#include <Logger/Logger.hpp>

namespace UServerUtils::Grpc::Core::Common::Logger
{

using Logger_var = Logging::Logger_var;

Logger_var set_logger(const Logger_var& logger);

} // namespace UServerUtils::Grpc::Core::Common::Logging

#endif //GRPC_CORE_COMMON_LOGGING_H_
