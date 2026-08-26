set(proj "TestDynAny")

add_idl(echoIDL echo.idl ${IDL_DIRECTORY}/tests/DynAny)

add_executable(${proj}
  test.cpp
)

target_compile_definitions(${proj}
  PRIVATE
    ORB_NAME=""
)

target_link_libraries(${proj}
  PRIVATE
    CORBACommons
    echoIDL
    CORBAConfigParser
)
add_test(
  NAME ${proj}
  COMMAND ${proj}
)
