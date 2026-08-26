set(proj "TestTextTemplate")

add_executable(${proj}
TestTemplate.cpp
)

target_link_libraries(${proj}
  PRIVATE
    Generics
    Sync
)
add_test(NAME ${proj}
         COMMAND ${proj})
