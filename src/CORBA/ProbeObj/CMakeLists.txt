#cmake_minimum_required (VERSION 2.6)




add_executable(ProbeObj
Application.cpp
)

target_link_libraries( ProbeObj CORBAConfigParser )


#install(TARGETS ProbeObj DESTINATION ${INSTALL_LIB})