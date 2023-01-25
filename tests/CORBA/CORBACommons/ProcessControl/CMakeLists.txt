#cmake_minimum_required (VERSION 2.6)

set(proj "TestProcessControl")


add_executable(${proj}
Application.cpp
)

target_link_libraries(${proj} CORBAConfigParser
#SimpleIDL TestCrashIDL
    )
add_test(NAME ${proj}
         COMMAND ${proj})

