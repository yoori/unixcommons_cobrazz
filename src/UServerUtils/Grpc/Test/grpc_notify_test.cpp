// GTEST
#include "gtest/gtest.h"

// STD
#include <grpcpp/grpcpp.h>
#include <grpcpp/notifier.h>

TEST (GrpcNotifyTest, Test1)
{
  auto completion_queue =
    std::make_unique<grpc::CompletionQueue>();
  auto notifier = std::make_unique<grpc::Notifier>();

  bool is_success = notifier->Notify(
    completion_queue.get(),
    reinterpret_cast<void*>(1));
  EXPECT_TRUE(is_success);
  void* tag1 = nullptr;
  bool ok1 = false;
  EXPECT_TRUE(completion_queue->Next(&tag1, &ok1));
  EXPECT_TRUE(ok1);
  EXPECT_EQ(reinterpret_cast<std::uintptr_t>(tag1), 1);

  is_success = notifier->Notify(
    completion_queue.get(),
    reinterpret_cast<void*>(2));
  EXPECT_TRUE(is_success);
  void* tag2 = nullptr;
  bool ok2 = false;
  EXPECT_TRUE(completion_queue->Next(&tag2, &ok2));
  EXPECT_TRUE(ok2);
  EXPECT_EQ(reinterpret_cast<std::uintptr_t>(tag2), 2);

  completion_queue->Shutdown();
  void* tag3 = nullptr;
  bool ok3 = false;
  EXPECT_FALSE(completion_queue->Next(&tag3, &ok3));

  is_success = notifier->Notify(
    completion_queue.get(),
    reinterpret_cast<void*>(3));
  EXPECT_FALSE(is_success);
}

TEST (GrpcNotifyTest, Test2)
{
  auto completion_queue =
    std::make_unique<grpc::CompletionQueue>();
  auto notifier = std::make_unique<grpc::Notifier>();

  bool is_success = notifier->Notify(
    completion_queue.get(),
    reinterpret_cast<void*>(1));
  EXPECT_TRUE(is_success);
  is_success = notifier->Notify(
    completion_queue.get(),
    reinterpret_cast<void*>(2));
  EXPECT_TRUE(is_success);

  completion_queue->Shutdown();
  is_success = notifier->Notify(
    completion_queue.get(),
    reinterpret_cast<void*>(3));
  EXPECT_FALSE(is_success);

  void* tag1 = nullptr;
  bool ok1 = false;
  EXPECT_TRUE(completion_queue->Next(&tag1, &ok1));
  EXPECT_EQ(reinterpret_cast<std::uintptr_t>(tag1), 1);
  EXPECT_TRUE(ok1);

  void* tag2 = nullptr;
  bool ok2 = false;
  EXPECT_TRUE(completion_queue->Next(&tag2, &ok2));
  EXPECT_EQ(reinterpret_cast<std::uintptr_t>(tag2), 2);
  EXPECT_TRUE(ok2);

  void* tag3 = nullptr;
  bool ok3 = false;
  EXPECT_FALSE(completion_queue->Next(&tag3, &ok3));
}