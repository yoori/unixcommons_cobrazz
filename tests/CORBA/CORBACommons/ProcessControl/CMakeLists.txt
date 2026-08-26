set(proj "TestProcessControl")

add_executable(${proj}
Application.cpp
)

target_link_libraries(${proj}
  PRIVATE
    TAO::IORTable
    CORBACommons
    Generics
    ProcessControlIDL
)
add_test(NAME ${proj}
         COMMAND ${proj})
