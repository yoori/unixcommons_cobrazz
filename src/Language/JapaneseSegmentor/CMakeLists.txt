set(proj "JapaneseSegmentor")
add_library(${proj}  SHARED
Mecab.cpp
)

target_link_libraries(${proj} Generics)
install(TARGETS ${proj} DESTINATION ${INSTALL_LIB})


