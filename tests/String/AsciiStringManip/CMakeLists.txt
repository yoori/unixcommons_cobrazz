set(proj "TestAsciiStringManip")

add_executable(${proj}
AsciiStringManipTest.cpp
)

target_link_libraries(${proj}
  PRIVATE
    Generics
)

add_test(NAME ${proj}
         COMMAND ${proj})
