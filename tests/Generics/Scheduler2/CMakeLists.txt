#cmake_minimum_required (VERSION 2.6)


#include_directories(   /usr/include/apr-1       )

set(proj "TestScheduler2")

add_executable(${proj}
Application.cpp

)
#add_library(PreloadACE  SHARED
#PreloadACE.cpp
#)

#link_directories()



target_link_libraries(${proj} Generics Logger)


add_test(NAME ${proj}
         COMMAND ${proj})
