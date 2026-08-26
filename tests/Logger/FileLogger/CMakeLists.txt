set(proj "TestFileLogger")

add_executable(${proj}
TestFileLogger.cpp

)

target_link_libraries(${proj}
  PRIVATE
    Generics
    Logger
    TestCommons2
)

add_test(NAME ${proj}
         COMMAND ${proj})
