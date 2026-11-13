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

add_executable(TestUuidThreadLocal UuidThreadLocalTest.cpp)
target_link_libraries(TestUuidThreadLocal PRIVATE Generics Threads::Threads)
target_link_options(TestUuidThreadLocal PRIVATE "-Wl,--export-dynamic")
add_test(NAME TestUuidThreadLocal COMMAND TestUuidThreadLocal)
