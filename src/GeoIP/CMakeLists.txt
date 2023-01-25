
set(proj Geoip)

add_library(Geoip  SHARED
IPMap.cpp
)

target_link_libraries(${proj} Generics GeoIP)
#install(TARGETS ${proj} DESTINATION ${INSTALL_LIB})


