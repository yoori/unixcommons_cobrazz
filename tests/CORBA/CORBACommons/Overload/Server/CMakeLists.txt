set(proj "CORBAOverloadServer")

add_idl(TestIntIDL TestInt.idl  ${IDL_DIRECTORY}/tests/Overload)

target_link_libraries(TestIntIDL
  PUBLIC
    StatsIDL
)

include_directories(${IDL_DIRECTORY})
add_executable(${proj}
Application.cpp
TestIntImpl.cpp
)

target_link_libraries(${proj}
  PRIVATE
    CORBACommons
    CORBAConfigParser
    Generics
    Logger
    ProcessControlIDL
    StatsIDL
    TestCommons
    TestIntIDL
)
add_test(NAME ${proj}
         COMMAND ${proj})
