
set(proj "TestAnalyzer")


add_executable(${proj}
AnalyzerTest.cpp
)


target_link_libraries(${proj} Generics Logger TestCommons2 String)
add_test(NAME ${proj}
         COMMAND ${proj})


