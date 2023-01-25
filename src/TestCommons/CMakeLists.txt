set(proj "TestCommons")
add_library(TestCommons  SHARED
  Error.cpp
  Memory.cpp

)
#install(TARGETS ${proj} DESTINATION ${INSTALL_LIB})