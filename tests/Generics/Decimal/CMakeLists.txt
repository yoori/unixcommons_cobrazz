set(proj "TestDecimal")

add_executable(${proj}
Application.cpp

)
target_link_libraries(${proj}
  PRIVATE
    Generics
)

add_test(NAME ${proj}
         COMMAND ${proj})
