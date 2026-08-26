set(proj "TestProcessLogger")

add_executable(${proj}

Application.cpp

)

target_link_libraries(${proj}
  PRIVATE
    Generics
    Logger
    TestCommons2
)

add_test(NAME ${proj}
         COMMAND ${proj})
