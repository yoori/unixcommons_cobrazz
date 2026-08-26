set(proj "TestHTTPCookie")

add_executable(${proj}
Application.cpp

)
target_link_libraries(${proj}
  PRIVATE
    Generics
    HTTP
)

add_test(NAME ${proj}
         COMMAND ${proj})
