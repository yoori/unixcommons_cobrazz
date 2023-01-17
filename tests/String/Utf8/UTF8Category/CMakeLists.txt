
set(proj "TestUTF8Category")


add_executable(${proj}
TestUTF8Category.cpp
)


target_link_libraries(${proj} Generics Logger TestCommons2 String)
add_test(NAME ${proj}
         COMMAND ${proj})


