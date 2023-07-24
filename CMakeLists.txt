cmake_minimum_required(VERSION 3.19)

project (unixcommons)

include(cmake/SetupEnvironment.cmake)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/)

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

set(Boost_NO_WARN_NEW_VERSIONS ON)

#find_package(Nghttp2 REQUIRED)
#find_package(Protobuf REQUIRED)
#find_package(gRPC REQUIRED)
#find_package(userver REQUIRED)
find_package(IDL REQUIRED)

include_directories(src src/CORBA)

set(IDL_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/IDL_COMMON)
include_directories(${IDL_DIRECTORY})

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(tests)

install(DIRECTORY bin/ DESTINATION bin USE_SOURCE_PERMISSIONS)
install(DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} DESTINATION bin)
install(DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} DESTINATION lib)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/share" DESTINATION . USE_SOURCE_PERMISSIONS)
