#ifndef GRPC_COMMON_LOGGING_H_
#define GRPC_COMMON_LOGGING_H_

// THIS
#include <Logger/Logger.hpp>

namespace UServerUtils::Grpc::Common::Logger
{

using Logger_var = Logging::Logger_var;

Logger_var set_logger(Logging::Logger* logger);

} // namespace UServerUtils::Grpc::Common::Logging

#endif //GRPC_COMMON_LOGGING_H_