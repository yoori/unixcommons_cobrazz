set(proj "LinguistUtil")
add_library(${proj}  SHARED
Application.cpp
)

target_link_libraries(${proj} Generics BLogic GenericSegmentor Polyglot)


install(TARGETS ${proj} DESTINATION ${INSTALL_LIB})