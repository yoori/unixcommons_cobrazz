
set(proj "TestSNMPSimple")

add_executable(${proj}

Application.cpp

)


target_link_libraries(${proj} Generics Logger TestCommons2 SNMPAgent)


add_test(NAME ${proj}
         COMMAND ${proj})
