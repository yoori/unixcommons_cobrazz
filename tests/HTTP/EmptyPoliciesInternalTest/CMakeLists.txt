set(proj "TestEmptPoliciesInt")

add_executable(${proj}
Main.cpp CommonClasses.cpp Tests.cpp
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
