set(proj "TestTimeManips")

add_executable(${proj}
Main.cpp

)
target_link_libraries(${proj}
  PRIVATE
    Generics
    Logger
    Sync
)

add_test(NAME ${proj}
         COMMAND ${proj})
