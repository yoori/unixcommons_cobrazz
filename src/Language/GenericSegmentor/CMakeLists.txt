set(proj "GenericSegmentor")
add_library(${proj}  SHARED
Polyglot.cpp
)

target_link_libraries(${proj} Generics)
#install(TARGETS ${proj} DESTINATION ${INSTALL_LIB})


