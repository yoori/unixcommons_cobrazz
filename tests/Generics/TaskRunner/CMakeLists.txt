set(proj "TestTaskRunner")

add_executable(${proj}
Main.cpp
)
target_link_libraries(${proj}
  PRIVATE
    Generics
)

add_test(NAME ${proj}
         COMMAND ${proj})
