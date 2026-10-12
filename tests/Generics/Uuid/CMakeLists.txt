set(proj "TestUuid")

add_executable(${proj}
UuidTest.cpp

)
target_link_libraries(${proj}
  PRIVATE
    Generics
    Logger
    Sync
    OpenSSL::Crypto
)

add_test(NAME ${proj}
         COMMAND ${proj})

add_executable(TestUuidComparison UuidComparisonTest.cpp)
target_link_libraries(TestUuidComparison PRIVATE Generics)
add_test(NAME TestUuidComparison COMMAND TestUuidComparison)
