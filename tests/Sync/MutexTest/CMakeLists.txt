set(proj "MutexTest")

add_executable(${proj}
Main.cpp
)

target_link_libraries(${proj}
  PRIVATE
    Threads::Threads
)
add_test(NAME ${proj}
         COMMAND ${proj})
