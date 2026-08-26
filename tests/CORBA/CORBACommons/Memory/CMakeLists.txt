#add_idl(CORBAInfResolveClientIDL client.idl)

set(proj "CORBAMemoryClient")

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
    TestIntIDL
)
add_test(NAME ${proj}
         COMMAND ${proj})
