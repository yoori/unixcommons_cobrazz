set(proj "TestStats")

#add_idl(TestInt2IDL TestInt2.idl ${IDL_DIRECTORY}/tests/SameProcess)

add_executable(${proj}
Application.cpp
)

target_link_libraries(${proj}
  PRIVATE
    CORBACommons
    Generics
    Logger
    ProcessControlIDL
    StatsIDL
    Sync
)
add_test(NAME ${proj}
         COMMAND ${proj})
