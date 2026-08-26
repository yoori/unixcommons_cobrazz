set(proj "TestHttpPoolSimple")

add_executable(${proj}
Main.cpp

)
target_link_libraries(${proj}
  PRIVATE
    Generics
    Logger
    Sync
    HTTP
    TestCommons
    HttpTestCommons
)

add_test(NAME ${proj}
         COMMAND ${proj})
