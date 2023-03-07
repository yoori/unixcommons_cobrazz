#ifndef GRPC_CORE_CLIENT_PENDING_QUEUE_H_
#define GRPC_CORE_CLIENT_PENDING_QUEUE_H_

// STD
#include <deque>
#include <queue>
#include <memory>

// PROTOBUF
#include <google/protobuf/message.h>

namespace UServerUtils::Grpc::Core::Client
{

enum class PendingQueueType
{
  Write = 0,
  WritesDone
};

using PendingQueueData = std::pair<
  PendingQueueType,
  std::unique_ptr<google::protobuf::Message>>;

using PendingQueue = std::queue<
  PendingQueueData,
  std::deque<PendingQueueData>>;

} // namespace UServerUtils::Grpc::Core::Client

#endif // GRPC_CORE_CLIENT_PENDING_QUEUE_H_
