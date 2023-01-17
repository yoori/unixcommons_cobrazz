#cmake_minimum_required (VERSION 2.6)


set(proj "CORBAConfigParser")

add_library(${proj}  SHARED
  CorbaConfigReader.cpp 
  ParameterConfig.cpp 
)
target_link_libraries(${proj} CORBACommons   xerces-c XMLUtility
    )

install(TARGETS ${proj} DESTINATION ${INSTALL_LIB})