#ifndef GRPC_CORE_SERVER_EVENT_TYPE_H_
#define GRPC_CORE_SERVER_EVENT_TYPE_H_

namespace UServerUtils::Grpc::Core::Server
{

enum class EventType
{
  Connection = 0,
  Read,
  Write,
  Finish,
  Done
};

} // UServerUtils::Grpc::Core::Server

#endif //GRPC_CORE_SERVER_EVENT_TYPE_H_
