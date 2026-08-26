set(proj "TestCRC")
add_executable(${proj}
${proj}.cpp

)
target_link_libraries(${proj}
  PRIVATE
    Generics
    Logger
)

add_test(NAME ${proj}
         COMMAND ${proj})
