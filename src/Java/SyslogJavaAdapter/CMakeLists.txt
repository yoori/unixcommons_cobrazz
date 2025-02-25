#cmake_minimum_required (VERSION 2.6)
#include_directories(   /usr/include/apr-1       )

add_library(SyslogJavaAdapter  SHARED
  JavaAdapter.cpp
)

target_include_directories(SyslogJavaAdapter
  PRIVATE
    /usr/lib/jvm/java-1.8.0-openjdk-1.8.0.362.b09-2.el8_7.x86_64/include/
    /usr/lib/jvm/java-1.8.0-openjdk-1.8.0.362.b09-2.el8_7.x86_64/include/linux/
)

#add_library(PreloadACE  SHARED
#PreloadACE.cpp
#)

#link_directories()

#target_link_libraries(SyslogJavaAdapter Stream event)


