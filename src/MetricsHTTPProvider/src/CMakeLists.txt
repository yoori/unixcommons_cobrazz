add_library(MetricsHTTPProvider SHARED
  MetricsHTTPProvider.cpp
  config_service.cpp
  )

if(APPLE)
  set(MACLINK "userver-stacktrace")
endif()

if(LINUX)
  set(LINUXLINK "Generics")
endif()

target_link_libraries(MetricsHTTPProvider
  userver-core 
  http_parser 
  ${MACLINK}
  ${LINUXLINK}
  pthread
  )
