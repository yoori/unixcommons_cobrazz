set(proj "CORBAObjectPoolServer")

add_idl(SimpleIDL Simple.idl ${IDL_DIRECTORY}/tests/ObjectPool)

add_executable(${proj}
ObjectPoolTestServer.cpp
)

target_link_libraries(${proj}
  PRIVATE
    CORBACommons
    Generics
    Logger
    ProcessControlIDL
    SimpleIDL
)
add_test(NAME ${proj}
         COMMAND ${proj})
