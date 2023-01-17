set(proj "Polyglot")
add_library(${proj} SHARED
DictionaryLoader.cpp
)

target_link_libraries(${proj} Generics)


install(TARGETS ${proj} DESTINATION ${INSTALL_LIB})