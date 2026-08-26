set(proj "TestCompStream")

add_executable(${proj}
Main.cpp

)

target_link_libraries(${proj}
  PRIVATE
    Generics
    Stream
)

add_test(NAME ${proj}
         COMMAND ${proj})
