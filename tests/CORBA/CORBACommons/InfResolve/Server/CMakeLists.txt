add_idl(CORBAInfResolveServerIDL server.idl ${IDL_DIRECTORY}/tests/InfResolve)

set(proj "CORBAInfResolveServer")

add_executable(${proj}
  TestServer.cpp
)

target_link_libraries(${proj}
  CORBACommons
  CORBAConfigParser
  CORBAInfResolveServerIDL
)

add_test(NAME ${proj} COMMAND ${proj})
