set(proj "CORBAObjectPoolInt")

add_executable(${proj}
ObjectPoolInt.cpp
)

target_link_libraries(${proj}
  PRIVATE
    CORBACommons
    Generics
    ProcessControlIDL
)
add_test(NAME ${proj}
         COMMAND ${proj})
