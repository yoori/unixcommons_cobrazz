set(proj "TestNormalizer")

add_executable(${proj}
Application.cpp
)

add_test(NAME ${proj}
         COMMAND ${proj})
