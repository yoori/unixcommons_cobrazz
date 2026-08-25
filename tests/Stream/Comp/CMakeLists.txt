
set(proj "TestCompStream")

add_executable(${proj}
Main.cpp

)


target_link_libraries(${proj} Generics Stream)


add_test(NAME ${proj}
         COMMAND ${proj})
