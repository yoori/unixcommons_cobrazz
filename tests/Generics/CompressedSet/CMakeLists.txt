set(proj "TestCompressedSet")

add_executable(${proj}
Application.cpp

)
add_test(NAME ${proj}
         COMMAND ${proj})
