set(proj "CORBAObjectPoolTestClient")

add_executable(${proj}
ObjectPoolTestClient.cpp
)

target_link_libraries(${proj}
  PRIVATE
    CORBACommons
    CORBAConfigParser
    Generics
    Logger
    ProcessControlIDL
    SimpleIDL
    Sync
)
add_test(NAME ${proj}
         COMMAND ${proj})
