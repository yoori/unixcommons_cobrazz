
set(proj "TestBlockFileAdapter")

add_executable(${proj}

Main.cpp

)


target_link_libraries(${proj} Generics PlainStorage)


add_test(NAME ${proj}
         COMMAND ${proj})
