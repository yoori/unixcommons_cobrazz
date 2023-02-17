add_executable(MetricsHTTPProviderTest 
  daemon_run1.cpp
)

include_directories()

if(APPLE)
set(MACLINK "userver-stacktrace")
endif()
if(LINUX)
set(LINUXLINK "Generics")
endif()

target_link_libraries(MetricsHTTPProviderTest UServerUtils)
