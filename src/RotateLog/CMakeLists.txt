set(proj "RotateLog")

add_library(${proj}  SHARED
    RotateLog.cpp
)

target_link_libraries(${proj}  Generics Logger)

#install(TARGETS ${proj} DESTINATION ${INSTALL_LIB})

