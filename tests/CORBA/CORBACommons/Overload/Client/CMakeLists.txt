#cmake_minimum_required (VERSION 2.6)


include_directories( ${IDL_DIRECTORY}  . )

set(proj "CORBAOverloadClient")

add_executable(${proj}
Application.cpp
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
    Sync
    TestIntIDL
)

add_test(NAME ${proj}
         COMMAND ${proj})
