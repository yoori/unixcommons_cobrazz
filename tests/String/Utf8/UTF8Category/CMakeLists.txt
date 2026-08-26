set(proj "TestUTF8Category")

add_executable(${proj}
TestUTF8Category.cpp
)

target_link_libraries(${proj}
  PRIVATE
    Generics
)
add_test(NAME ${proj}
         COMMAND ${proj})
