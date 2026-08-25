#cmake_minimum_required (VERSION 2.6)

set(proj "CORBAObjectPoolInt")


add_executable(${proj}
ObjectPoolInt.cpp
)

target_link_libraries(${proj}
  PRIVATE
    TAO::PortableServer
    TAO::Valuetype
    CORBACommons
    Generics
    ProcessControlIDL
)
add_test(NAME ${proj}
         COMMAND ${proj})
