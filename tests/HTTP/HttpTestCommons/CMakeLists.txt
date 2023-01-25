#cmake_minimum_required (VERSION 2.6)


#include_directories(   /usr/include/apr-1       )

set(proj "HttpTestCommons")

add_library(${proj} SHARED
 CommonClasses.cpp
           ApachePorts.cpp
)
#add_library(ApacheModule  SHARED
#Module.cpp

#)
#add_library(PreloadACE  SHARED
#PreloadACE.cpp
#)

#link_directories()



target_link_libraries(${proj} Generics Logger HTTP TestCommons)


add_test(NAME ${proj}
         COMMAND ${proj})
