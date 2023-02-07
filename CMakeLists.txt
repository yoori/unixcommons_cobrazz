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
find_package(USERVER REQUIRED)

find_package(IDL REQUIRED)

set(USERVER ${CMAKE_CURRENT_SOURCE_DIR}/3dparty/userver)

include_directories(src src/CORBA)

set(IDL_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/IDL_COMMON)
include_directories(${IDL_DIRECTORY})

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(tests)

install(DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} DESTINATION bin)
install(DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} DESTINATION lib)
