include(FetchContent)
set(USE_RTTI 1 CACHE INTERNAL "USE_RTTI enable")
FetchContent_Declare(rocksdb
  GIT_REPOSITORY https://github.com/arbogdanov/rocksdb.git
  GIT_TAG master
)
FetchContent_MakeAvailable(rocksdb)

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
  ${rocksdb_SOURCE_DIR}/include
  Grpc/Core
)

set (SOURCES_FILES
  Grpc/ClientFactory.cpp
  Grpc/CobrazzServerBuilder.cpp
  Grpc/ComponentsBuilder.cpp
  Grpc/Core/Common/Logging.cpp
  Grpc/Core/Common/Scheduler.cpp
  Grpc/Core/Server/CommonContextCoro.cpp
  Grpc/Core/Server/RpcPoolImpl.cpp
  Grpc/Core/Server/Server.cpp
  Grpc/Core/Server/ServerCoro.cpp
  Grpc/Core/Server/Service.cpp
  Grpc/FileManager/File.cpp
  Grpc/FileManager/FileManager.cpp
  Grpc/FileManager/FileManagerPool.cpp
  Grpc/FileManager/IoUring.cpp
  Grpc/FileManager/Semaphore.cpp
  Grpc/RocksDB/DataBase.cpp
  Grpc/RocksDB/DataBaseManager.cpp
  Grpc/RocksDB/DataBaseManagerPool.cpp
  Grpc/RocksDB/Logger.cpp
  Grpc/Logger.cpp
  Grpc/Manager.cpp
  Grpc/RegistratorDynamicSettings.cpp
  Grpc/Server.cpp
  Grpc/ServerBuilder.cpp
  Grpc/TaskProcessorContainer.cpp
  Grpc/TaskProcessorContainerBuilder.cpp
  GRPCServer.cpp
)

add_library(UServerUtils STATIC
  ${SOURCES_FILES}
)

add_library(UServerUtilsMetrics SHARED
  ConfigDistributor.cpp
  MetricsHTTPProvider.cpp
  UServerConfigService.cpp
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

target_link_libraries(UServerUtilsMetrics
  Generics
  Logger
  pthread
  userver-core
)



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
  Grpc/Test/file_manager_test.cpp
  Grpc/Test/grpc_cobrazz_stream_stream_client_async_test.cpp
  Grpc/Test/grpc_cobrazz_stream_stream_client_coro_test.cpp
  Grpc/Test/grpc_cobrazz_stream_stream_server_async_test.cpp
  Grpc/Test/grpc_cobrazz_stream_stream_server_coro_test.cpp
  Grpc/Test/grpc_cobrazz_stream_unary_server_async_test.cpp
  Grpc/Test/grpc_cobrazz_unary_stream_server_async_test.cpp
  Grpc/Test/grpc_cobrazz_unary_unary_client_async_test.cpp
  Grpc/Test/grpc_cobrazz_unary_unary_server_async_test.cpp
  Grpc/Test/grpc_cobrazz_unary_unary_server_coro_test.cpp
  Grpc/Test/grpc_notify_test.cpp
  Grpc/Test/rocsdb_test.cpp
  Grpc/Test/grpc_test_main.cpp
  Grpc/Test/grpc_userver_test1.cpp
  Grpc/Test/grpc_userver_test2.cpp
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
    Grpc/Test/${FileName}.cpp
  )
  target_link_libraries(${Target}
    ${Boost_LIBRARIES}
    UServerUtils
    UServerUtils_proto
  )
endforeach()