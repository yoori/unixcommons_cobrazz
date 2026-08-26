add_library(Geoip SHARED
  IPMap.cpp
)

target_link_libraries(Geoip
  PUBLIC
    Generics
    GeoIP::GeoIP
    Sync
)
