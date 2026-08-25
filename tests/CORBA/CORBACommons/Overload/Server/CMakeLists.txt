#cmake_minimum_required (VERSION 2.6)

set(proj "CORBAOverloadServer")

add_idl(TestIntIDL TestInt.idl  ${IDL_DIRECTORY}/tests/Overload)

target_link_libraries(TestIntIDL PRIVATE StatsIDL)

include_directories(${IDL_DIRECTORY})
add_executable(${proj}
Application.cpp
TestIntImpl.cpp
)

target_link_libraries(${proj}
  PRIVATE
    TAO::PortableServer
    TAO::Valuetype
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
