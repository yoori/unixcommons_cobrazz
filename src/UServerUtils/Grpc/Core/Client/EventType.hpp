#ifndef GRPC_CORE_CLIENT_EVENT_TYPE_H_
#define GRPC_CORE_CLIENT_EVENT_TYPE_H_

namespace UServerUtils::Grpc::Core::Client
{

enum class EventType
{
  Start = 0,
  Initialize,
  Read,
  Write,
  Finish,
  Stop
};

} // UServerUtils::Grpc::Core::Client

#endif //GRPC_CORE_CLIENT_EVENT_TYPE_H_