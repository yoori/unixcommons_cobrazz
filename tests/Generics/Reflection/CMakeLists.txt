set(proj "TestReflection")

add_executable(${proj}
Application.cpp

)
add_test(NAME ${proj}
         COMMAND ${proj})
