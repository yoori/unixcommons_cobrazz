add_idl(CORBAInfResolveClientIDL client.idl ${IDL_DIRECTORY}/tests/InfResolve)

set(proj "CORBAInfResolveClient")

add_executable(${proj}
  TestClient.cpp
)

target_link_libraries(${proj}
  PRIVATE
    TAO::TAO
    CORBAInfResolveClientIDL
)

add_test(NAME ${proj}
  COMMAND ${proj})
