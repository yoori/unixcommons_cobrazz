include(FetchContent)

cmake_policy(SET CMP0079 NEW)

set(USERVER_NAMESPACE userver CACHE STRING "Open C++ namespace to use")
set(USERVER_NAMESPACE_BEGIN "namespace ${USERVER_NAMESPACE} {" CACHE STRING "Open C++ namespace to use")
set(USERVER_NAMESPACE_END "}" CACHE STRING "Close C++ namespace to use")

find_package(absl)
if (NOT absl_FOUND)
  message(SEND_ERROR "Failed to find absl")
  return()
else()
  message(STATUS "Using absl ${absl_VERSION}")
endif()

find_package(Protobuf)
if (NOT Protobuf_FOUND)
  message(SEND_ERROR "Failed to find Protobuf")
  return()
else()
  message(STATUS "Using protobuf ${Protobuf_VERSION}")
  set(GRPC_PROTOBUF_INCLUDE_DIRS "${Protobuf_INCLUDE_DIR}" CACHE INTERNAL "")
endif()

find_program(PROTOBUF_PROTOC protoc)
if(NOT PROTOBUF_PROTOC)
  message(SEND_ERROR "Failed to find protoc")
  return()
else()
  message("-- Using protoc: ${PROTOBUF_PROTOC}")
endif()

find_package(gRPC)
if (NOT gRPC_FOUND)
  message(SEND_ERROR "Failed to find GRPC")
  return()
else()
  message(STATUS "Using gRPC ${gRPC_VERSION}")
  set(GRPC_GRPCPP gRPC::grpc++)
  set(GRPC_EXTERNAL_ENABLE 1)
endif()

find_program(GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
if (NOT GRPC_CPP_PLUGIN_EXECUTABLE)
  message(SEND_ERROR "Failed to find grpc_cpp_plugin")
  return()
else()
  message("-- Using grpc_cpp_plugin: ${GRPC_CPP_PLUGIN_EXECUTABLE}")
  set(PROTO_GRPC_CPP_PLUGIN ${GRPC_CPP_PLUGIN_EXECUTABLE} CACHE INTERNAL "")
endif()

find_package(GTest REQUIRED)
find_package(Boost REQUIRED COMPONENTS thread)

include_directories (
  Grpc/Core
  ${GRPC_PROTOBUF_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
)

get_property(
  compile_options
  DIRECTORY
  PROPERTY COMPILE_OPTIONS
)

set_property(
  DIRECTORY
  APPEND
  PROPERTY COMPILE_OPTIONS -Wno-error
)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.10.0)
FetchContent_MakeAvailable(googletest)

option(USERVER_FEATURE_STACKTRACE "Build userver feature stacktrace" OFF)
option(USERVER_FEATURE_POSTGRESQL "Provide asynchronous driver for PostgreSQL" OFF)
option(USERVER_FEATURE_MONGODB "Provide asynchronous driver for MongoDB" OFF)
option(USERVER_FEATURE_CLICKHOUSE "Provide asynchronous driver for ClickHouse" OFF)
option(USERVER_FEATURE_RABBITMQ "Provide asynchronous driver for RabbitMQ" OFF)
FetchContent_Declare(
  userver
  GIT_REPOSITORY https://github.com/arbogdanov/userver.git
  GIT_TAG cobrazz
)
FetchContent_MakeAvailable(userver)

set_property(
  DIRECTORY
  PROPERTY COMPILE_OPTIONS ${compile_options}
)

unset(compile_options)

set (SOURCES_FILES
  Grpc/Core/Common/Logging.cpp
  Grpc/Core/Common/Scheduler.cpp
  Grpc/Core/Server/CommonContextCoro.cpp
  Grpc/Core/Server/RpcPoolImpl.cpp
  Grpc/Core/Server/Server.cpp
  Grpc/Core/Server/ServerCoro.cpp
  Grpc/Core/Server/Service.cpp
  Grpc/CobrazzServerBuilder.cpp
  Grpc/ComponentsBuilder.cpp
  Grpc/ClientFactory.cpp
  Grpc/Logger.cpp
  Grpc/Manager.cpp
  Grpc/Server.cpp
  Grpc/ServerBuilder.cpp
  Grpc/TaskProcessorContainer.cpp
  Grpc/TaskProcessorContainerBuilder.cpp
  ConfigDistributor.cpp
  GRPCServer.cpp
  MetricsHTTPProvider.cpp
  UServerConfigService.cpp
)

add_library(UServerUtils STATIC
  ${SOURCES_FILES}
)

target_link_libraries(UServerUtils
  ${GRPC_GRPCPP}
  userver-core
  userver-grpc
  http_parser
  Generics
  Logger
  pthread
)

target_compile_definitions(UServerUtils PUBLIC
  "USERVER_NAMESPACE=${USERVER_NAMESPACE}"
  "USERVER_NAMESPACE_BEGIN=${USERVER_NAMESPACE_BEGIN}"
  "USERVER_NAMESPACE_END=${USERVER_NAMESPACE_END}")

set(USERVER_INCLUDE_DIRS
  ${USERVER_PUBLIC_INCLUDE_DIRS}
  ${userver_SOURCE_DIR}/third_party/moodycamel/include
  CACHE INTERNAL "")

# UnitTests
include(${CMAKE_CURRENT_SOURCE_DIR}/Grpc/Generator/GrpcTargets.cmake)
add_grpc_library(UServerUtils_proto
  PROTOS
    echo.proto
    test1.proto
    test2.proto
    test.proto
    test_coro_client.proto
  SOURCE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/Grpc/Test/Proto
)

add_executable(TestUserver
  Grpc/Test/grpc_cobrazz_unary_unary_client_async_test.cpp
  Grpc/Test/grpc_cobrazz_unary_unary_server_async_test.cpp
  Grpc/Test/grpc_cobrazz_unary_unary_server_coro_test.cpp
  Grpc/Test/grpc_cobrazz_stream_stream_client_async_test.cpp
  Grpc/Test/grpc_cobrazz_stream_stream_client_coro_test.cpp
  Grpc/Test/grpc_cobrazz_unary_stream_server_async_test.cpp
  Grpc/Test/grpc_cobrazz_stream_stream_server_async_test.cpp
  Grpc/Test/grpc_cobrazz_stream_stream_server_coro_test.cpp
  Grpc/Test/grpc_cobrazz_stream_unary_server_async_test.cpp
  Grpc/Test/grpc_notify_test.cpp
  Grpc/Test/grpc_userver_test1.cpp
  Grpc/Test/grpc_userver_test2.cpp
  Grpc/Test/grpc_test_main.cpp
)

target_link_libraries(TestUserver
  GTest::gtest
  GTest::gtest_main
  UServerUtils
  UServerUtils_proto
  ${PROTOBUF_LIBRARY}
)

add_test(TestUserver TestUserver)

# Benchmarks
foreach(_Target
        Grpc_Benchmark1
        Grpc_Benchmark2
        Grpc_Benchmark_Sleep)
  string(TOLOWER ${_Target} FileName)
  add_executable(${_Target}
    Grpc/Test/${FileName}.cpp
  )
  target_link_libraries(${_Target}
    UServerUtils
    UServerUtils_proto
    ${Boost_LIBRARIES}
  )
endforeach()

foreach(_Target
        Grpc_Cobrazz_Async_Benchmark
        Grpc_Cobrazz_Coro_Benchmark
        Grpc_Client_Remote_Test
        Grpc_Server_Remote_Test)
  string(TOLOWER ${_Target} FileName)
  add_executable(${_Target}
    ${SOURCES_FILES}
    Grpc/Test/${FileName}.cpp
  )
  target_link_libraries(${_Target}
    ${GRPC_GRPCPP}
    userver-core
    userver-grpc
    http_parser
    Generics
    Logger
    pthread
    UServerUtils_proto
  )
endforeach()