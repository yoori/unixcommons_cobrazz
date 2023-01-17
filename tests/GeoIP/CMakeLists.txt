#cmake_minimum_required (VERSION 2.6)


#include_directories(   /usr/include/apr-1       )

set(proj "TestGeoIP")

add_executable(${proj}
TestGeoIP.cpp
)
#add_library(ApacheModule  SHARED
#Module.cpp

#)
#add_library(PreloadACE  SHARED
#PreloadACE.cpp
#)

#link_directories()



target_link_libraries(${proj} Generics Logger Geoip)


add_test(NAME ${proj}
         COMMAND ${proj})
