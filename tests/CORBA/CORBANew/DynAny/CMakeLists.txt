#cmake_minimum_required (VERSION 2.6)

set(proj "TestDynAny")

add_idl(echoIDL echo.idl ${IDL_DIRECTORY}/tests/DynAny)

add_definitions (-DORB_NAME=\"\")
add_executable(${proj}
test.cpp
)

target_link_libraries(${proj} CORBACommons echoIDL CORBAConfigParser
    )
add_test(NAME ${proj}
         COMMAND ${proj})

