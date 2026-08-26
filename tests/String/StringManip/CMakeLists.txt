set(proj "TestStringManip")

add_executable(${proj}
Application.cpp
)

target_link_libraries(${proj}
  PRIVATE
    Generics
)

add_test(NAME ${proj}
         COMMAND ${proj})

set(proj "StringManipJsonEscapePerfTest")

add_executable(${proj}
JsonEscapePerfTest.cpp
)

target_link_libraries(${proj}
  PRIVATE
    Generics
)

set(proj "StringManipStrToIntPerfTest")

add_executable(${proj}
StrToIntPerfTest.cpp
)

target_link_libraries(${proj}
  PRIVATE
    Generics
)
