set(proj "Profiler")

add_library(${proj}  SHARED
    Lib/Profiler.cpp
    Parser/Parser.cpp
)
target_link_libraries(${proj} pthread)
#install(TARGETS ${proj} DESTINATION ${INSTALL_LIB})


