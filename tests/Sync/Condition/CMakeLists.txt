set(proj "TestConditionVariable")

add_executable(${proj}
Application.cpp
)

target_link_libraries(${proj}
  PRIVATE
    Sync
)
add_test(NAME ${proj}
         COMMAND ${proj})
