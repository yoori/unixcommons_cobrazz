// GRPC
#include <grpc/grpc.h>

// GTEST
#include <gtest/gtest.h>

// PROTOBUF
#include <google/protobuf/stubs/common.h>

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);

  const auto result = RUN_ALL_TESTS();

  google::protobuf::ShutdownProtobufLibrary();
  grpc_shutdown_blocking();

  return result;
}
