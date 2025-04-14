cmake_minimum_required(VERSION 3.19)

set(CMAKE_CXX_COMPILER  /opt/rh/gcc-toolset-10/root/usr/bin/g++)
set(CMAKE_C_COMPILER /opt/rh/gcc-toolset-10/root/usr/bin/gcc)

project (unixcommons)

include(cmake/SetupEnvironment.cmake)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(UNIX AND NOT APPLE)
SET(LINUX true)
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(CTest)
endif()

set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules
)

if(UNIX AND NOT APPLE)
  SET(LINUX true)
endif()

if(LINUX)
  set(CMAKE_SHARED_LINKER_FLAGS "-Xlinker --export-dynamic -Xlinker -zdefs")
else()
endif()

set(Boost_NO_WARN_NEW_VERSIONS ON)

if (NOT DEFINED USE_USERVER_PACKAGE)
  set(USE_USERVER_PACKAGE FALSE)
endif()

add_library(atomic INTERFACE)

set(JAVA_INCLUDE_DIRECTORIES
  /usr/lib/jvm/java-1.8.0-openjdk-1.8.0.442.b06-2.0.1.el8.x86_64/include/
  /usr/lib/jvm/java-1.8.0-openjdk-1.8.0.442.b06-2.0.1.el8.x86_64/include/linux/
)

find_package(ACE REQUIRED)
find_package(BZip2 REQUIRED)
find_package(Boost REQUIRED COMPONENTS thread)
find_package(EV REQUIRED)
find_package(Event REQUIRED)
find_package(GTest REQUIRED)
find_package(GeoIP REQUIRED)
find_package(Iconv REQUIRED)
find_package(IDL REQUIRED)
find_package(NetSNMP REQUIRED)
find_package(Nghttp2 REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(PCRE REQUIRED)
find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)
find_package(XercesC REQUIRED)
find_package(gRPC REQUIRED)
find_package(uring REQUIRED)
include(cmake/SetupUserver.cmake)

SET(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/") # workaround for snappy-devel installation bug
find_package(RocksDB REQUIRED)

include(${CMAKE_CURRENT_SOURCE_DIR}/src/UServerUtils/Grpc/Generator/GrpcTargets.cmake)

include_directories(src src/CORBA)

set(IDL_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/IDL_COMMON)
include_directories(${IDL_DIRECTORY})

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(tests)

install(DIRECTORY bin/ DESTINATION bin USE_SOURCE_PERMISSIONS)
install(DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} DESTINATION bin)
install(DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} DESTINATION lib)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/share" DESTINATION . USE_SOURCE_PERMISSIONS)
