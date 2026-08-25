#cmake_minimum_required (VERSION 2.6)


#include_directories(   /usr/include/apr-1       )

set(proj "TestReflection")

add_executable(${proj}
Application.cpp

)
#add_library(ApacheModule  SHARED
#Module.cpp

#)
#add_library(PreloadACE  SHARED
#PreloadACE.cpp
#)

#link_directories()





add_test(NAME ${proj}
         COMMAND ${proj})
