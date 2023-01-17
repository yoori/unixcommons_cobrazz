
set(proj "MutexTest")


add_executable(${proj}
Main.cpp
)


target_link_libraries(${proj} Generics Logger TestCommons2 String ACE)
add_test(NAME ${proj}
         COMMAND ${proj})


