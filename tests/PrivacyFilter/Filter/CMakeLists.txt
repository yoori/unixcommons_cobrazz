set(proj "TestFilter")

add_executable(${proj}

Application.cpp

)

target_link_libraries(${proj}
  PRIVATE
    PrivacyFilter
)

add_test(NAME ${proj}
         COMMAND ${proj})
