#cmake_minimum_required (VERSION 2.6)




add_executable(MpTest 

daemon_run1.cpp

)
include_directories(
                )

if(APPLE)
set(MACLINK "userver-stacktrace")
endif()
if(LINUX)
set(LINUXLINK "Generics")
endif()

target_link_libraries(MpTest  MetricsHTTPProvider)


