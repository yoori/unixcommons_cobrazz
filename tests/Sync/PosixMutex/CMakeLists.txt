
set(proj "TestPosixMutex")


add_executable(${proj}
TestPosixMutex.cpp
)


target_link_libraries(${proj} Generics Logger TestCommons2 String)
add_test(NAME ${proj}
         COMMAND ${proj})


