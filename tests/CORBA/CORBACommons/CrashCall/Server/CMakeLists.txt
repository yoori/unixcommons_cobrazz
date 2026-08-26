set(proj "TestCrashServer")

add_idl(TestCrashIDL TestCrash.idl ${IDL_DIRECTORY}/tests/CrashCall)

add_executable(${proj}
Application.cpp
)

target_link_libraries(${proj}
  PRIVATE
    CORBACommons
    CORBAConfigParser
    Generics
    Logger
    ProcessControlIDL
    TestCrashIDL
)
add_test(NAME ${proj}
         COMMAND ${proj})
