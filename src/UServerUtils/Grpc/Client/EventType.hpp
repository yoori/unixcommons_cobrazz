#ifndef GRPC_CLIENT_EVENT_TYPE_H_
#define GRPC_CLIENT_EVENT_TYPE_H_

namespace UServerUtils::Grpc::Client
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

} // UServerUtils::Grpc::Client

#endif //GRPC_CLIENT_EVENT_TYPE_H_