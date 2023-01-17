#cmake_minimum_required (VERSION 2.6)

set(proj "TestFew_endpoints_tooling")

add_idl(echoIDL1 echo.idl ${IDL_DIRECTORY}/tests/TestFew_endpoints_tooling)

add_executable(${proj}
test.cpp
)

target_link_libraries(${proj} CORBACommons echoIDL1 CORBAConfigParser
    )
add_test(NAME ${proj}
         COMMAND ${proj})

