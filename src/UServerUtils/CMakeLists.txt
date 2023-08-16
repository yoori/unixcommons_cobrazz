find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
find_package(Nghttp2 REQUIRED) # include Nghttp2 local cmake for correct userver linkage
find_package(userver REQUIRED)

find_program(PROTOBUF_PROTOC protoc)
if(NOT PROTOBUF_PROTOC)
  message(SEND_ERROR "Failed to find protoc")
  return()
else()
  message("-- Using protoc: ${PROTOBUF_PROTOC}")
endif()

# find grpc_cpp_plugin : part of userver distribution
find_program(GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
if (NOT GRPC_CPP_PLUGIN_EXECUTABLE)
  message(SEND_ERROR "Failed to find grpc_cpp_plugin")
  return()
else()
  message("-- Using grpc_cpp_plugin: ${GRPC_CPP_PLUGIN_EXECUTABLE}")
  set(PROTO_GRPC_CPP_PLUGIN ${GRPC_CPP_PLUGIN_EXECUTABLE} CACHE INTERNAL "")
endif()

include_directories (
  Grpc/Core
  ${GRPC_PROTOBUF_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
)

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
  Grpc/RegistratorDynamicSettings.cpp
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

# link userver before other libraries, because it can be static (DSO require shared linkage after)
target_link_libraries(UServerUtils
  userver-core
  userver-grpc
  ${GRPC_GRPCPP}
  ${PROTOBUF_LIBRARY}
  http_parser
  Generics
  Logger
  pthread
  Boost::thread
)

#target_compile_definitions(UServerUtils PUBLIC
#  "USERVER_NAMESPACE=${USERVER_NAMESPACE}"
#  "USERVER_NAMESPACE_BEGIN=${USERVER_NAMESPACE_BEGIN}"
#  "USERVER_NAMESPACE_END=${USERVER_NAMESPACE_END}")

#set(USERVER_INCLUDE_DIRS
#  ${USERVER_PUBLIC_INCLUDE_DIRS}
#  ${userver_SOURCE_DIR}/third_party/moodycamel/include
#  CACHE INTERNAL "")


# UnitTests
include(${CMAKE_CURRENT_SOURCE_DIR}/Grpc/Generator/GrpcTargets.cmake)

add_grpc_library(UServerUtils_proto
  PROTOS
    echo.proto
    test.proto
    test1.proto
    test2.proto
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
)

add_test(TestUserver TestUserver)

# Benchmarks
foreach(Target
  Grpc_Benchmark1
  Grpc_Benchmark2
  Grpc_Benchmark_Sleep)
  string(TOLOWER ${Target} FileName)
  add_executable(${Target}
    Grpc/Test/${FileName}.cpp
  )
  target_link_libraries(${Target}
    UServerUtils
    UServerUtils_proto
    ${Boost_LIBRARIES}
  )
endforeach()

foreach(Target
  Grpc_Cobrazz_Async_Benchmark
  Grpc_Cobrazz_Coro_Benchmark
  Grpc_Client_Remote_Test
  Grpc_Server_Remote_Test)
  string(TOLOWER ${Target} FileName)
  add_executable(${Target}
    ${SOURCES_FILES}
    Grpc/Test/${FileName}.cpp
  )
  target_link_libraries(${Target}
    userver-core
    userver-grpc
    ${Boost_LIBRARIES}
    ${GRPC_GRPCPP}
    http_parser
    Generics
    Logger
    pthread
    UServerUtils_proto
    Boost::thread
  )
endforeach()
