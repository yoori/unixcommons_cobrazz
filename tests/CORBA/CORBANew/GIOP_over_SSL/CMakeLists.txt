#cmake_minimum_required (VERSION 2.6)

set(proj "GIOP_over_SSL")

add_idl(echoIDL echo.idl ${IDL_DIRECTORY}/tests/GIOP_over_SSL)

add_executable(${proj}
test.cpp
test_client.cpp
)

target_link_libraries(${proj} CORBACommons echoIDL CORBAConfigParser
    )
add_test(NAME ${proj}
         COMMAND ${proj})

