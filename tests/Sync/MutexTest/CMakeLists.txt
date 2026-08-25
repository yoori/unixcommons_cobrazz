
set(proj "MutexTest")


add_executable(${proj}
Main.cpp
)


target_link_libraries(${proj} ACE::ACE)
add_test(NAME ${proj}
         COMMAND ${proj})
