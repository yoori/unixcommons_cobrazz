cmake_minimum_required(VERSION 3.19)

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

add_library(atomic INTERFACE) # workaround for third party libraries that try to link it
find_package(ACE REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(XercesC REQUIRED)
find_package(BZip2 REQUIRED)
find_package(GeoIP REQUIRED)
find_package(Event REQUIRED)
find_package(NetSNMP REQUIRED)
find_package(PCRE REQUIRED)
find_package(EV REQUIRED)
find_package(Threads REQUIRED)
find_package(Iconv REQUIRED)
find_package(GTest REQUIRED)
set(FOROS_BOOST_ROOT "/opt/foros/boost185" CACHE PATH "Boost installation root")
if(EXISTS "${FOROS_BOOST_ROOT}/include/boost/version.hpp")
  set(BOOST_ROOT "${FOROS_BOOST_ROOT}" CACHE PATH "Boost root" FORCE)
  set(Boost_ROOT "${FOROS_BOOST_ROOT}" CACHE PATH "Boost root" FORCE)
  set(Boost_DIR
    "${FOROS_BOOST_ROOT}/lib64/cmake/Boost-1.85.0"
    CACHE PATH "Boost CMake config directory"
    FORCE)
  set(BOOST_INCLUDEDIR
    "${FOROS_BOOST_ROOT}/include"
    CACHE PATH "Boost include directory"
    FORCE)
  set(Boost_INCLUDE_DIR
    "${FOROS_BOOST_ROOT}/include"
    CACHE PATH "Boost include directory"
    FORCE)
  set(BOOST_LIBRARYDIR
    "${FOROS_BOOST_ROOT}/lib64"
    CACHE PATH "Boost library directory"
    FORCE)
  set(Boost_LIBRARY_DIR_DEBUG
    "${FOROS_BOOST_ROOT}/lib64"
    CACHE PATH "Boost debug library directory"
    FORCE)
  set(Boost_LIBRARY_DIR_RELEASE
    "${FOROS_BOOST_ROOT}/lib64"
    CACHE PATH "Boost release library directory"
    FORCE)
  set(Boost_THREAD_LIBRARY_DEBUG
    "${FOROS_BOOST_ROOT}/lib64/libboost_thread.so"
    CACHE FILEPATH "Boost thread debug library"
    FORCE)
  set(Boost_THREAD_LIBRARY_RELEASE
    "${FOROS_BOOST_ROOT}/lib64/libboost_thread.so"
    CACHE FILEPATH "Boost thread release library"
    FORCE)
  set(Boost_NO_SYSTEM_PATHS ON CACHE BOOL "Use only configured Boost root" FORCE)
endif()
set(Boost_NO_WARN_NEW_VERSIONS ON)
find_package(Boost REQUIRED COMPONENTS thread)
if(EXISTS "${FOROS_BOOST_ROOT}/include")
  include_directories(SYSTEM "${FOROS_BOOST_ROOT}/include")
endif()
if(EXISTS "${FOROS_BOOST_ROOT}/lib64")
  link_directories("${FOROS_BOOST_ROOT}/lib64")
endif()

set(JAVA_INCLUDE_DIRECTORIES
  /usr/lib/jvm/java-1.8.0-openjdk-1.8.0.442.b06-2.0.1.el8.x86_64/include/
  /usr/lib/jvm/java-1.8.0-openjdk-1.8.0.442.b06-2.0.1.el8.x86_64/include/linux/
)

find_package(IDL REQUIRED)
find_package(Nghttp2 REQUIRED)
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
set(DUSERVER_FEATURE_TESTSUITE OFF)
#find_package(userver REQUIRED)

include_directories(src src/CORBA)

set(IDL_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/IDL_COMMON)
include_directories(${IDL_DIRECTORY})

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(tests)

install(DIRECTORY bin/ DESTINATION bin USE_SOURCE_PERMISSIONS)
install(DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} DESTINATION bin)
install(DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} DESTINATION lib)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/share" DESTINATION . USE_SOURCE_PERMISSIONS)
