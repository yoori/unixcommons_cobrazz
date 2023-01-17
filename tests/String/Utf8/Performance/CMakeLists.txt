
set(proj "TestUTF8Performance")


add_executable(${proj}
PerformanceTest.cpp
)


target_link_libraries(${proj} Generics Logger TestCommons2 String)
add_test(NAME ${proj}
         COMMAND ${proj})


