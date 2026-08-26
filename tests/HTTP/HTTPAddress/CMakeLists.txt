set(proj "TestHTTPAddress")

add_executable(${proj}
Application.cpp

)
target_link_libraries(${proj}
  PRIVATE
    HTTP
)

add_test(NAME ${proj}
         COMMAND ${proj})
