include(FetchContent)

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
  ${Boost_INCLUDE_DIRS}
  ${GRPC_PROTOBUF_INCLUDE_DIRS}
  Grpc/Core
)

if (NOT ${USE_USERVER_PACKAGE})
  set_property(DIRECTORY APPEND PROPERTY COMPILE_OPTIONS -Wno-error)
  set(GRPC_EXTERNAL_ENABLE TRUE CACHE INTERNAL "")

  option(USERVER_FEATURE_GRPC "Build userver feature grpc" ON)
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

  target_include_directories(userver-core
    PUBLIC ${userver_SOURCE_DIR}/third_party/uboost_coro/include/
  )
endif()

set (SOURCES_FILES
  Grpc/Common/Logging.cpp
  Grpc/Common/Scheduler.cpp
  Grpc/Server/CommonContextCoro.cpp
  Grpc/Server/RpcPoolImpl.cpp
  Grpc/Server/Server.cpp
  Grpc/Server/ServerBuilder.cpp
  Grpc/Server/ServerCoro.cpp
  Grpc/Server/Service.cpp
  Http/Client/Client.cpp
  Http/Client/Request.cpp
  Http/Server/HttpHandler.cpp
  Http/Server/HttpServer.cpp
  Http/Server/HttpServerBuilder.cpp
  Http/Server/MonitorHandler.cpp
  UServerGrpc/ClientFactory.cpp
  UServerGrpc/Server.cpp
  UServerGrpc/ServerBuilder.cpp
  Statistics/MemoryStatisticsProvider.cpp
  ComponentsBuilder.cpp
  Logger.cpp
  Manager.cpp
  RegistratorDynamicSettings.cpp
  TaskProcessorContainer.cpp
  TaskProcessorContainerBuilder.cpp
)

add_library(UServerUtils STATIC
  ${SOURCES_FILES}
)

# link userver before other libraries, because it can be
# static (DSO require shared linkage after)
target_link_libraries(UServerUtils
  ${GRPC_GRPCPP}
  ${PROTOBUF_LIBRARY}
  Boost::thread
  Generics
  Logger
  http_parser
  pthread
  rocksdb
  uring
  userver-core
  userver-grpc
)

target_compile_definitions(UServerUtils PUBLIC ENABLE_READ_IFSTREAM)

add_grpc_library(UServerUtils_proto
  PROTOS
    echo.proto
    test.proto
    test1.proto
    test2.proto
    test_coro_client.proto
  SOURCE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/Test/Proto
)

add_executable(TestUserver
  Test/file_manager_test.cpp
  Test/grpc_cobrazz_stream_stream_client_async_test.cpp
  Test/grpc_cobrazz_stream_stream_client_coro_test.cpp
  Test/grpc_cobrazz_stream_stream_server_async_test.cpp
  Test/grpc_cobrazz_stream_stream_server_coro_test.cpp
  Test/grpc_cobrazz_stream_unary_server_async_test.cpp
  Test/grpc_cobrazz_unary_stream_server_async_test.cpp
  Test/grpc_cobrazz_unary_unary_client_async_test.cpp
  Test/grpc_cobrazz_unary_unary_server_async_test.cpp
  Test/grpc_cobrazz_unary_unary_server_coro_test.cpp
  Test/grpc_notify_test.cpp
  Test/main.cpp
  Test/grpc_userver_test.cpp
  Test/http_test.cpp
  Test/rocsdb_test.cpp
)

target_link_libraries(TestUserver
  GTest::gtest
  GTest::gtest_main
  UServerUtils
  UServerUtils_proto
  userver-core
  userver-grpc
  uring
)

add_test(TestUserver TestUserver)

# Benchmarks
foreach(Target
  File_Manager_Benchmark
  File_Reader_Benchmark
  Grpc_Benchmark
  Grpc_Client_Remote_Test
  Grpc_Cobrazz_Async_Benchmark
  Grpc_Cobrazz_Coro_Benchmark
  Grpc_Server_Remote_Test)
  string(TOLOWER ${Target} FileName)
  add_executable(${Target}
    Test/${FileName}.cpp
  )
  target_link_libraries(${Target}
    ${Boost_LIBRARIES}
    UServerUtils
    UServerUtils_proto
  )
endforeach()
