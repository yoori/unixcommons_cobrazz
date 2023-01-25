set(proj "KoreanSegmentor")
add_library(${proj}  SHARED
  Korean.cpp
  Klt.cpp
  Moran.cpp


)

target_link_libraries(${proj} Generics)

#install(TARGETS ${proj} DESTINATION ${INSTALL_LIB})


