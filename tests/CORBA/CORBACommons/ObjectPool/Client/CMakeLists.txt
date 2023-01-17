#cmake_minimum_required (VERSION 2.6)

set(proj "CORBAObjectPoolTestClient")


add_executable(${proj}
ObjectPoolTestClient.cpp
)

target_link_libraries(${proj} SimpleIDL TestCrashIDL CORBAConfigParser
    )
add_test(NAME ${proj}
         COMMAND ${proj})

