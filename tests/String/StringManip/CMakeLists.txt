
set(proj "TestStringManip")

add_executable(${proj}
Application.cpp
)


target_link_libraries(${proj} Generics Logger TestCommons2 String)


add_test(NAME ${proj}
         COMMAND ${proj})

set(proj "StringManipJsonEscapePerfTest")

add_executable(${proj}
JsonEscapePerfTest.cpp
)

target_link_libraries(${proj} Generics Logger String)
