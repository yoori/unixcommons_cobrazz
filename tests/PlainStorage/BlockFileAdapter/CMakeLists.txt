set(proj "TestBlockFileAdapter")

add_executable(${proj}

Main.cpp

)

target_link_libraries(${proj}
  PRIVATE
    Generics
    PlainStorage
)

add_test(NAME ${proj}
         COMMAND ${proj})
