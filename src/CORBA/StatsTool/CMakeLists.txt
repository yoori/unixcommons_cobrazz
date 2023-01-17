#cmake_minimum_required (VERSION 2.6)




add_library(StatsTool  SHARED
    Application.cpp
)

target_link_libraries( StatsTool CORBACommons )

install(TARGETS ${proj} DESTINATION ${INSTALL_LIB})
