set (proj "ChineeseSegmentor")
add_library(${proj}  SHARED
NLPIR.cpp
  
)

target_link_libraries(${proj}  Generics)


install(TARGETS ${proj} DESTINATION ${INSTALL_LIB})