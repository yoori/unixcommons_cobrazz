set(proj "CaseUtil")

set(proj "CaseUtil")
add_library(${proj}  SHARED
${proj}.cpp
)


target_link_libraries(${proj} Generics)


install(TARGETS ${proj} DESTINATION ${INSTALL_LIB})