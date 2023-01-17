set(proj "Sync")

add_library(${proj}  SHARED
  Condition.cpp 

)
install(TARGETS ${proj} DESTINATION ${INSTALL_LIB})