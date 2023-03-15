include(FetchContent)
option(USERVER_FEATURE_STACKTRACE "Build userver feature stacktrace" OFF)
option(USERVER_FEATURE_POSTGRESQL "Provide asynchronous driver for PostgreSQL" OFF)
option(USERVER_FEATURE_MONGODB "Provide asynchronous driver for MongoDB" OFF)
option(USERVER_FEATURE_CLICKHOUSE "Provide asynchronous driver for ClickHouse" OFF)
FetchContent_Declare(
  userver
  GIT_REPOSITORY https://github.com/arbogdanov/userver.git
  GIT_TAG grpc
)
FetchContent_MakeAvailable(userver)

find_package(GTest REQUIRED)
find_package(Boost REQUIRED COMPONENTS thread)
include_directories(${Boost_INCLUDE_DIRS})

add_library(UServerUtils SHARED
  #MetricsHTTPProvider.cpp
  #UServerConfigService.cpp
  #GRPCServer.cpp
  #ConfigDistributor.cpp
  Grpc/ComponentsBuilder.cpp
  Grpc/ClientFactory.cpp
  Grpc/Logger.cpp
  Grpc/Manager.cpp
  Grpc/Server.cpp
  Grpc/ServerBuilder.cpp
  Grpc/TaskProcessorContainer.cpp
  Grpc/TaskProcessorContainerBuilder.cpp
)

target_link_libraries(UServerUtils
  userver-core
  userver-grpc
  http_parser
  Generics
  Logger
  pthread
)

# UnitTests
include(${CMAKE_CURRENT_SOURCE_DIR}/Grpc/Generator/GrpcTargets.cmake)
add_grpc_library(UServerUtils_proto
  PROTOS
    test1.proto
    test2.proto
  SOURCE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/Grpc/Test/Proto
)

add_executable(TestUserver
  Grpc/Test/grpc_test.cpp
)

target_link_libraries(TestUserver
  GTest::gtest
  GTest::gtest_main
  UServerUtils
  UServerUtils_proto
)

add_test(TestUserver TestUserver)

# Benchmarks
add_executable(GrpcBenchmark
  Grpc/Test/grpc_benchmark.cpp
)

target_link_libraries(GrpcBenchmark
  UServerUtils
  UServerUtils_proto
  ${Boost_LIBRARIES}
)

# http service
add_executable(HttpUserver
  HelloService/HelloService.cpp
)

target_link_libraries(HttpUserver
  UServerUtils
  UServerUtils_proto
  ${Boost_LIBRARIES}
)