
set(proj "TestUTF8Stress")


add_executable(${proj}
StressTest.cpp
)


target_link_libraries(${proj} Generics Stream)
add_test(NAME ${proj}
         COMMAND ${proj})
