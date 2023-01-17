set(proj "PlainStorage")

add_library(${proj}  SHARED
  BlockFileAdapter.cpp 
  Map.cpp 


)
target_link_libraries(${proj} Generics)


install(TARGETS ${proj} DESTINATION ${INSTALL_LIB})