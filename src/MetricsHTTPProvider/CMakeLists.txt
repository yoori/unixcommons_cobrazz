cmake_minimum_required(VERSION 3.19)

project (MetricsHTTPProvider)





include_directories( /usr/local/include src)



set(USERVER_NAMESPACE_BEGIN "namespace userver {" CACHE STRING "Open C++ namespace to use")
set(USERVER_NAMESPACE_END "}" CACHE STRING "Close C++ namespace to use")

add_compile_definitions(
  "USERVER_NAMESPACE=userver"
  "USERVER_NAMESPACE_BEGIN=${USERVER_NAMESPACE_BEGIN}"
  "USERVER_NAMESPACE_END=${USERVER_NAMESPACE_END}"
)



ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(test)

    

