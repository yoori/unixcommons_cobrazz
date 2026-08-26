set(proj "TestLastPtr")

add_executable(${proj}
Application.cpp

)
target_link_libraries(${proj}
  PRIVATE
    Generics
    Sync
)

add_test(NAME ${proj}
         COMMAND ${proj})
