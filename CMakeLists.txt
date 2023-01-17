cmake_minimum_required(VERSION 3.19)

project (unixcommons)

if(UNIX AND NOT APPLE)
SET(LINUX true)
endif()

if(LINUX)
    set(CMAKE_PREFIX_PATH $ENV{HOME}/Qt/5.15.2/gcc_64)
    set(CMAKE_SHARED_LINKER_FLAGS "-Xlinker --export-dynamic -Xlinker -zdefs")
else()
    set(CMAKE_PREFIX_PATH $ENV{HOME}/Qt/5.15.2/clang_64)
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(CTest)
endif()
 set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules
  )

set(INSTALL_DIR "~/unixcommons-cobrazz-bin2")
set(INSTALL_LIB ${INSTALL_DIR}/lib)
set(INSTALL_BIN  ${INSTALL_DIR}/bin)


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

#find_package(OpenDDS REQUIRED)
include_directories( /usr/local/include src src/CORBA)

set(IDL_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/IDL_COMMON)
include_directories(${IDL_DIRECTORY})



set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC ")
link_directories( /usr/local/lib  )


ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(tests)

    

