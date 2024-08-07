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

set (SOURCES_FILES
  FileManager/File.cpp
  FileManager/FileManager.cpp
  FileManager/FileManagerPool.cpp
  FileManager/IoUring.cpp
  FileManager/Semaphore.cpp
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
  RocksDB/DataBase.cpp
  RocksDB/DataBaseManager.cpp
  RocksDB/DataBaseManagerPool.cpp
  RocksDB/Logger.cpp
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
  Test/grpc_userver_test1.cpp
  Test/grpc_userver_test2.cpp
  Test/http_test.cpp
  Test/rocsdb_test.cpp
)

target_link_libraries(TestUserver
  GTest::gtest
  GTest::gtest_main
  UServerUtils
  UServerUtils_proto
  uring
)

add_test(TestUserver TestUserver)

# Benchmarks
foreach(Target
  File_Manager_Benchmark
  Grpc_Benchmark1
  Grpc_Benchmark2
  Grpc_Benchmark_Sleep
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