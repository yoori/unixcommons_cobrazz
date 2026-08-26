set(proj "TestListener")

add_executable(${proj}
Application.cpp

)
target_link_libraries(${proj}
  PRIVATE
    Generics
    Logger
    Sync
)

add_test(NAME ${proj}
         COMMAND ${proj})
