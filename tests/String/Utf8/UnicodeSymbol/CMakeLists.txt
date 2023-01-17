
set(proj "TestUnicodeSymbol")


add_executable(${proj}
TestUnicodeSymbol.cpp
)


target_link_libraries(${proj} Generics Logger TestCommons2 String Stream)
add_test(NAME ${proj}
         COMMAND ${proj})


