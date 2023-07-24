#ifndef GRPC_CORE_SERVER_PENDING_QUEUE_H_
#define GRPC_CORE_SERVER_PENDING_QUEUE_H_

// STD
#include <memory>
#include <queue>
#include <memory>
#include <optional>
#include <tuple>

// GRPC
#include <grpcpp/support/status.h>

// PROTOBUF
#include <google/protobuf/message.h>

namespace UServerUtils::Grpc::Core::Server
{

enum class PendingQueueType
{
  Write = 0,
  Finish,
  Stop
};

using PendingQueueData = std::tuple<
  PendingQueueType,
  std::unique_ptr<google::protobuf::Message>,
  std::optional<grpc::Status>>;

using PendingQueue = std::queue<
  PendingQueueData,
  std::deque<PendingQueueData>>;

} // namespace UServerUtils::Grpc::Core::Server

#endif //GRPC_CORE_SERVER_PENDING_QUEUE_H_
