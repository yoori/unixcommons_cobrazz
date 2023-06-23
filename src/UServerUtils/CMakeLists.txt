#include_directories(/usr/local/include src)


set(USERVER_NAMESPACE_BEGIN "namespace userver {" CACHE STRING "Open C++ namespace to use")
set(USERVER_NAMESPACE_END "}" CACHE STRING "Close C++ namespace to use")

add_compile_definitions(
    "USERVER_NAMESPACE=userver"
    "USERVER_NAMESPACE_BEGIN=${USERVER_NAMESPACE_BEGIN}"
    "USERVER_NAMESPACE_END=${USERVER_NAMESPACE_END}"
)


add_library(UServerUtils SHARED
            MetricsHTTPProvider.cpp
            UServerConfigService.cpp
            GRPCServer.cpp
            ConfigDistributor.cpp
#  CompositeMetricsProvider.cpp
           )

if(APPLE)
    set(MACLINK "userver-stacktrace")
endif()

if(LINUX)
  set(LINUXLINK "Generics")
endif()

target_link_libraries(UServerUtils
                              userver-core
                              userver-grpc
                              http_parser
                              ${MACLINK}
                              ${LINUXLINK}
                              pthread
                             )



#ADD_SUBDIRECTORY(sample)