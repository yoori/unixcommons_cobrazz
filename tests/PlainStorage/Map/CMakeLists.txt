set(proj "TestPlainMap")

add_executable(${proj}
Main.cpp

)

target_link_libraries(${proj}
  PRIVATE
    Generics
    Logger
    Sync
    PlainStorage
)

add_test(NAME ${proj}
         COMMAND ${proj})
