set(proj "TestSNMPSimple")

add_executable(${proj}
  Application.cpp
)

target_link_libraries(${proj} Generics Logger SNMPAgent)

add_test(NAME ${proj} COMMAND ${proj})
