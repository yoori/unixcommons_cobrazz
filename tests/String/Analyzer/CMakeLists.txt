set(proj "TestAnalyzer")

add_executable(${proj}
AnalyzerTest.cpp
)

target_link_libraries(${proj}
  PRIVATE
    Generics
    Logger
)
add_test(NAME ${proj}
         COMMAND ${proj})
