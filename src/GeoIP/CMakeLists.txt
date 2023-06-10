add_library(Geoip SHARED
  IPMap.cpp
)

target_link_libraries(Geoip Generics GeoIP)


