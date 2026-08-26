set(proj "TestGeoIP")

add_executable(${proj}
TestGeoIP.cpp
)
target_link_libraries(${proj}
  PRIVATE
    Geoip
)

add_test(NAME ${proj}
         COMMAND ${proj})
