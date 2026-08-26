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
