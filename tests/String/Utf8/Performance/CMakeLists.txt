set(proj "TestUTF8Performance")

add_executable(${proj}
PerformanceTest.cpp
)

target_link_libraries(${proj}
  PRIVATE
    Generics
)
add_test(NAME ${proj}
         COMMAND ${proj})
