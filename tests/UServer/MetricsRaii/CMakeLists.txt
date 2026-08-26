add_executable(MetricsRAIITest 
  sampleMetricsRAII.cpp
)
if(APPLE)
set(MACLINK "userver-stacktrace")
endif()
if(LINUX)
set(LINUXLINK "Generics")
endif()

target_link_libraries(MetricsRAIITest
  PRIVATE
    UServerUtils
)
