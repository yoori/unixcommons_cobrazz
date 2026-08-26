set(proj "TestIsProperty")

add_executable(${proj}
IsProperty.cpp
)

target_link_libraries(${proj}
  PRIVATE
    Generics
)
add_test(NAME ${proj}
         COMMAND ${proj})
