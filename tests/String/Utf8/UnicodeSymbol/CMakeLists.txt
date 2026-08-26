set(proj "TestUnicodeSymbol")

add_executable(${proj}
TestUnicodeSymbol.cpp
)

target_link_libraries(${proj}
  PRIVATE
    Generics
    Stream
)
add_test(NAME ${proj}
         COMMAND ${proj})
