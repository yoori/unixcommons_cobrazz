cmake_minimum_required(VERSION 3.19)

project(unixcommons)

include(cmake/SetupEnvironment.cmake)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  include(CTest)
endif()

set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  add_link_options(
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:LINKER:-z,defs>"
  )
endif()

add_library(atomic INTERFACE) # workaround for third party libraries that try to link it
find_package(ACE REQUIRED)
find_package(TAO REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(XercesC REQUIRED)
find_package(BZip2 REQUIRED)
find_package(ZLIB REQUIRED)
find_package(GeoIP REQUIRED)
find_package(Event REQUIRED)
find_package(NetSNMP REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PCRE2 REQUIRED IMPORTED_TARGET libpcre2-8)
find_package(EV REQUIRED)
find_package(Threads REQUIRED)
find_package(Iconv REQUIRED)
find_package(GTest REQUIRED)
find_package(JNI REQUIRED)
set(FOROS_BOOST_ROOT "/opt/foros/boost185" CACHE PATH "Boost installation root")
if(NOT EXISTS "${FOROS_BOOST_ROOT}/include/boost/version.hpp")
  message(FATAL_ERROR "Boost headers not found in ${FOROS_BOOST_ROOT}")
endif()

set(Boost_DIR
  "${FOROS_BOOST_ROOT}/lib64/cmake/Boost-1.85.0"
  CACHE PATH "Boost CMake config directory"
  FORCE)

find_package(Boost 1.85.0 CONFIG REQUIRED COMPONENTS system thread)

if(NOT TARGET Boost::headers)
  message(FATAL_ERROR "Boost::headers target not found")
endif()

get_target_property(
  BOOST_HEADERS_COMPILE_OPTIONS
  Boost::headers
  INTERFACE_COMPILE_OPTIONS)
if(NOT BOOST_HEADERS_COMPILE_OPTIONS)
  set(BOOST_HEADERS_COMPILE_OPTIONS)
endif()
if(NOT "-msse2" IN_LIST BOOST_HEADERS_COMPILE_OPTIONS)
  set_property(TARGET Boost::headers APPEND PROPERTY INTERFACE_COMPILE_OPTIONS -msse2)
endif()
unset(BOOST_HEADERS_COMPILE_OPTIONS)

find_package(IDL REQUIRED)
find_package(Nghttp2 REQUIRED)
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
set(DUSERVER_FEATURE_TESTSUITE OFF)

include_directories(src src/CORBA)

set(IDL_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/IDL_COMMON)
include_directories(${IDL_DIRECTORY})

add_subdirectory(src)
add_subdirectory(tests)

install(DIRECTORY bin/ DESTINATION bin USE_SOURCE_PERMISSIONS)
install(DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} DESTINATION bin)
install(DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} DESTINATION lib)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/share" DESTINATION . USE_SOURCE_PERMISSIONS)
