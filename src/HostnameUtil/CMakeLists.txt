
set(proj "HostnameUtil")
add_library(${proj}  SHARED
HostnameUtil.cpp

)

target_link_libraries(${proj} Generics)
#install(TARGETS ${proj} DESTINATION ${INSTALL_LIB})


