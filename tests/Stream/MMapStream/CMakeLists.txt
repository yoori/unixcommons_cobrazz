
set(proj "TestMMapStream")

add_executable(${proj}
Main.cpp

)


target_link_libraries(${proj} Generics)


add_test(NAME ${proj}
         COMMAND ${proj})
