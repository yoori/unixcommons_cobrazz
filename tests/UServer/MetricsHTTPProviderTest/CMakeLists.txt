add_executable(MetricsHTTPProviderTest 
  sampleMetrics.cpp
)
add_executable(MetricsHTTPProviderPrometheusTest 
  sampleMetricsPrometheus.cpp
)
if(APPLE)
set(MACLINK "userver-stacktrace")
endif()
if(LINUX)
set(LINUXLINK "Generics")
endif()

target_link_libraries(MetricsHTTPProviderTest
  PRIVATE
    UServerUtils
)

target_link_libraries(MetricsHTTPProviderPrometheusTest
  PRIVATE
    UServerUtils
)
