#cmake_minimum_required (VERSION 2.6)




add_library(ProbeObj  SHARED
Application.cpp
)

target_link_libraries( ProbeObj CORBAConfigParser )


install(TARGETS ${proj} DESTINATION ${INSTALL_LIB})