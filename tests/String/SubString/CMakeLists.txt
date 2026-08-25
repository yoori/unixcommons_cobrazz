
set(proj "TestSubString")


add_executable(${proj}
Application.cpp
)


add_test(NAME ${proj}
         COMMAND ${proj})
