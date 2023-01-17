
set(proj "TestPlainMap")

add_executable(${proj}
Main.cpp

)


target_link_libraries(${proj} Generics Logger TestCommons2 PlainStorage)


add_test(NAME ${proj}
         COMMAND ${proj})
