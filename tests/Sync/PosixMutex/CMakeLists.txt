
set(proj "TestPosixMutex")


add_executable(${proj}
TestPosixMutex.cpp
)


target_link_libraries(${proj} Generics)
add_test(NAME ${proj}
         COMMAND ${proj})
