set(proj "TestUTF8Validation")

add_executable(${proj}
  TestUTF8Validation.cpp
)

add_test(NAME ${proj} COMMAND ${proj})
