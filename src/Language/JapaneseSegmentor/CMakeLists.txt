set(proj "JapaneseSegmentor")
add_library(${proj}  SHARED
Mecab.cpp
)

target_link_libraries(${proj}
  PRIVATE
  Sync)
#install(TARGETS ${proj} DESTINATION ${INSTALL_LIB})
