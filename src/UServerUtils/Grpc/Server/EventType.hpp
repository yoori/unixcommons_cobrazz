#ifndef GRPC_SERVER_EVENT_TYPE_H_
#define GRPC_SERVER_EVENT_TYPE_H_

namespace UServerUtils::Grpc::Server
{

enum class EventType
{
  Connection = 0,
  Read,
  Write,
  Finish,
  Done
};

} // UServerUtils::Grpc::Server

#endif //GRPC_SERVER_EVENT_TYPE_H_
