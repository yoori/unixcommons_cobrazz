include_directories( ${IDL_DIRECTORY}  . )

set(proj "CORBACrashCallClient")

add_executable(${proj}
Application.cpp
)

add_dependencies(${proj} TestCrashIDL)

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
